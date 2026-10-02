#include "pch.hpp"
#include "items.hpp"
#include "world.hpp"
#include "onVariant/SetClothing.hpp"
#include "onVariant/CountryState.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "commands/punch.hpp"
#include "tools/string.hpp"

#include "peer.hpp"

bool peer::exists(const std::string &growid)
{
    ::hStmt hStmt{ "SELECT 1 FROM peer WHERE growid = ? LIMIT 1" };

    MYSQL_BIND param = make_bind_in(growid); // WHERE
    hStmt.bind_param(&param);
    hStmt.execute();

    return (!mysql_stmt_store_result(hStmt.pStmt) &&
            mysql_stmt_num_rows(hStmt.pStmt) > 0);
}

::blob slot::to_blob() const
{
    ::blob blob;
    blob.i16(this->id);
    blob.i16(this->count);

    return blob;
}

template<typename T>
void peer::mysql_insert(const std::string &column, const T &value)
{
    ::hStmt hStmt{
        std::format(
            "INSERT INTO peer ({}) VALUES (?)",
            column
        ).c_str()
    };

    MYSQL_BIND param = make_bind_in(value);
    hStmt.bind_param(&param);
    hStmt.execute();
}

template void peer::mysql_insert<signed>(
    const std::string&,
    const signed&
);

template void peer::mysql_insert<unsigned>(
    const std::string&,
    const unsigned&
);

template void peer::mysql_insert<float>(
    const std::string&,
    const float&
);

template void peer::mysql_insert<std::string>(
    const std::string&,
    const std::string&
);

template<typename T>
void peer::mysql_update(
    const std::string &column,
    const T &value
)
{
    ::hStmt hStmt{
        std::format(
            "UPDATE peer SET {} = ? WHERE growid = ?",
            column
        ).c_str()
    };

    MYSQL_BIND params[2] = {
        make_bind_in(value),
        make_bind_in(this->growid)
    };

    hStmt.bind_param(params);
    hStmt.execute();
}

template void peer::mysql_update<signed>(
    const std::string&,
    const signed&
);

template void peer::mysql_update<unsigned>(
    const std::string&,
    const unsigned&
);

template void peer::mysql_update<float>(
    const std::string&,
    const float&
);

template void peer::mysql_update<std::string>(
    const std::string&,
    const std::string&
);

template<typename T>
T peer::mysql_select(
    const std::string &column,
    const std::string &arg
)
{
    T value{};

    ::hStmt hStmt{
        std::format(
            "SELECT {}({}) FROM peer WHERE growid = ? LIMIT 1",
            arg,
            column
        ).c_str()
    };

    MYSQL_BIND param = make_bind_in(this->growid);
    hStmt.bind_param(&param);

    u_long length = 0;

    MYSQL_BIND result = make_bind_out(value);
    result.length = &length;

    mysql_stmt_bind_result(
        hStmt.pStmt,
        &result
    );

    hStmt.execute();
    hStmt.fetch();

    if constexpr (std::is_same_v<T, std::string>)
        value.resize(length);

    return value;
}

/*
 * ============================================================
 * LOAD ALL PLAYER DATA
 * ============================================================
 */

void peer::mysql_select_all()
{
    this->user_id =
        this->mysql_select<signed>("uid");

    this->growid =
        this->mysql_select<std::string>("growid");

    this->password =
        this->mysql_select<std::string>("password");

    this->created_at =
        this->mysql_select<std::time_t>(
            "created_at",
            "UNIX_TIMESTAMP"
        );

    auto blob =
        this->mysql_select<std::vector<u_char>>(
            "inventory"
        );

    const u_char *u8 = blob.data();

    int pos{};

    memcpy(
        &this->slot_size,
        u8 + pos,
        sizeof(int)
    );

    pos += sizeof(int);

    short size{};

    memcpy(
        &size,
        u8 + pos,
        sizeof(short)
    );

    pos += sizeof(short);

    this->slots.resize(size);

    for (::slot &slot : this->slots)
    {
        memcpy(
            &slot.id,
            u8 + pos,
            sizeof(short)
        );

        pos += sizeof(short);

        memcpy(
            &slot.count,
            u8 + pos,
            sizeof(short)
        );

        pos += sizeof(short);
    }
}

/*
 * ============================================================
 * SERIALIZE INVENTORY
 * ============================================================
 */

::blob peer::serialize_inventory() const
{
    ::blob blob{};

    blob.i32(this->slot_size);
    blob.i16(this->slots.size());

    for (const ::slot &slot : this->slots)
    {
        blob.push_back(
            slot.to_blob()
        );
    }

    return blob;
}

/*
 * ============================================================
 * PLAYER LOAD
 * ============================================================
 */

void peer::load(
    const std::string &growid,
    const std::string &password
)
{
    /*
     * ----------------------------------------------------------
     * CREATE ACCOUNT IF IT DOES NOT EXIST
     * ----------------------------------------------------------
     */

    if (!this->exists(growid))
    {
        this->mysql_insert(
            "growid",
            growid
        );

        this->mysql_update(
            "password",
            password
        );

        this->slots.resize(3ull);

        this->slots[0ull] =
            ::slot{18, 1};      // Fist

        this->slots[1ull] =
            ::slot{32, 1};      // Wrench

        this->slots[2ull] =
            ::slot{9640, 1};    // World Lock

        this->mysql_update<std::vector<u_char>>(
            "inventory",
            this->serialize_inventory().data()
        );
    }

    /*
     * ----------------------------------------------------------
     * LOAD ACCOUNT DATA FROM DATABASE
     * ----------------------------------------------------------
     */

    this->mysql_select_all();

    /*
     * ==========================================================
     * HAKAN / SERVER OWNER
     *
     * ROLEX = DEVELOPER
     *
     * Only the GrowID "ROLEX" receives DEVELOPER role.
     * Every other account remains PLAYER.
     * ==========================================================
     */

    if (this->growid == "ROLEX")
    {
        this->role = DEVELOPER;
    }
    else
    {
        this->role = PLAYER;
    }
}

/*
 * ============================================================
 * PLAYER DESTRUCTOR
 * ============================================================
 */

peer::~peer()
{
    this->mysql_update<std::vector<u_char>>(
        "inventory",
        this->serialize_inventory().data()
    );
}

/*
 * ============================================================
 * INVENTORY
 * ============================================================
 */

u_short peer::emplace(::slot slot)
{
    if (
        auto it =
            std::ranges::find(
                this->slots,
                slot.id,
                &::slot::id
            );
        it != this->slots.end()
    )
    {
        const u_short excess =
            std::max(
                0,
                (it->count + slot.count) - 200
            );

        it->count =
            std::min(
                it->count + slot.count,
                200
            );

        if (it->count == 0)
        {
            const ::item &item =
                id_to_item(it->id);

            if (
                item.cloth_type !=
                clothing::NONE
            )
            {
                this->clothing[
                    item.cloth_type
                ] = 0;
            }
        }

        return excess;
    }
    else
    {
        this->slots.emplace_back(
            std::move(slot)
        );
    }

    return 0;
}

/*
 * ============================================================
 * XP
 * ============================================================
 */

void peer::add_xp(
    ENetEvent &event,
    u_short value
)
{
    u_short &lvl =
        this->level.front();

    u_short &xp =
        this->level.back() += value;

    for (; lvl < 125; )
    {
        u_short xp_formula =
            50 * (lvl * lvl + 2);

        if (xp < xp_formula)
            break;

        xp -= xp_formula;
        lvl++;

        if (lvl == 50)
        {
            modify_item_inventory(
                event,
                ::slot{1400, 1}
            );
        }

        if (lvl == 125)
        {
            on::CountryState(event);
        }

        send_varlist(
            event.peer,
            {
                "OnPlayerLeveledUp",
                lvl
            }
        );

        send_varlist(
            event.peer,
            {
                "OnParticleEffect",
                46u,
                CL_Vec2f{
                    1812.0f,
                    1724.0f
                },
                0.0f,
                0.0f
            }
        );

        std::string message =
            std::format(
                "{} is now level {}!",
                this->display_growid,
                lvl
            );

        send_varlist(
            event.peer,
            {
                "OnTalkBubble",
                this->netid,
                message,
                0u
            }
        );

        on::ConsoleMessage(
            event.peer,
            message
        );
    }
}

/*
 * ============================================================
 * CLOTHING EFFECTS
 * ============================================================
 */

void peer::update_effects()
{
    this->punch_effect = 0;

    for (float cloth : this->clothing)
    {
        u_char punch_id =
            get_punch_id(
                (u_int)cloth
            );

        if (punch_id != 0)
        {
            this->punch_effect =
                punch_id;
        }
    }
}

/*
 * ============================================================
 * GLOBAL ENET HOST
 * ============================================================
 */

ENetHost *host;

/*
 * ============================================================
 * GET PEERS
 * ============================================================
 */

std::vector<ENetPeer*> peers(
    const std::string &world,
    peer_condition condition,
    std::function<void(ENetPeer&)> fun
)
{
    std::vector<ENetPeer*> _peers{};

    _peers.reserve(
        host->peerCount
    );

    for (
        ENetPeer &peer :
        std::span(
            host->peers,
            host->peerCount
        )
    )
    {
        if (
            peer.state ==
            ENET_PEER_STATE_CONNECTED
        )
        {
            if (
                condition ==
                peer_condition::PEER_SAME_WORLD
            )
            {
                ::peer *pOthers =
                    static_cast<::peer*>(
                        peer.data
                    );

                if (
                    pOthers->netid == 0 ||
                    pOthers->recent_worlds.back() != world
                )
                {
                    continue;
                }
            }

            fun(peer);

            _peers.push_back(
                &peer
            );
        }
    }

    return _peers;
}

/*
 * ============================================================
 * SAFE DISCONNECT
 * ============================================================
 */

void safe_disconnect_peers(int code)
{
    peers(
        "",
        peer_condition::PEER_ALL,
        [](ENetPeer &p)
        {
            enet_peer_disconnect(
                &p,
                0
            );
        }
    );

    enet_host_flush(host);

    enet_host_destroy(host);

    host = nullptr;

    enet_deinitialize();
}

/*
 * ============================================================
 * GAME PACKET
 * ============================================================
 */

gamePacket make_gamePacket(
    const enet_uint8 *data
)
{
    const int *i32 =
        reinterpret_cast<const int*>(data);

    const u_int *u32 =
        reinterpret_cast<const u_int*>(data);

    const float *f32 =
        reinterpret_cast<const float*>(data);

    return gamePacket{
        .type = i32[1],
        .netid = i32[2],
        .uid = i32[3],
        .state = i32[4],
        .count = f32[5],
        .id = i32[6],

        .pos =
            ::pos{
                f32[7],
                f32[8]
            },

        .speed =
            ::pos{
                f32[9],
                f32[10]
            },

        .idk = f32[11],

        .punch =
            ::pos{
                i32[12],
                i32[13]
            },

        .size = u32[14]
    };
}

/*
 * ============================================================
 * COMPRESS GAME PACKET
 * ============================================================
 */

::blob compress_state(
    const gamePacket &gamePacket
)
{
    ::blob blob{};

    blob.i32(
        gamePacket.packet_create
    );

    blob.i32(
        gamePacket.type
    );

    blob.i32(
        gamePacket.netid
    );

    blob.i32(
        gamePacket.uid
    );

    blob.i32(
        gamePacket.state
    );

    blob.f32(
        gamePacket.count
    );

    blob.i32(
        gamePacket.id
    );

    blob.f32(
        gamePacket.pos.x
    );

    blob.f32(
        gamePacket.pos.y
    );

    blob.f32(
        gamePacket.speed.x
    );

    blob.f32(
        gamePacket.speed.y
    );

    blob.f32(
        gamePacket.idk
    );

    blob.i32(
        gamePacket.punch.x
    );

    blob.i32(
        gamePacket.punch.y
    );

    blob.i32(
        gamePacket.size
    );

    return blob;
}

/*
 * ============================================================
 * SEND INVENTORY STATE
 * ============================================================
 */

void send_inventory_state(
    ENetEvent &event
)
{
    ::peer *pPeer =
        static_cast<::peer*>(
            event.peer->data
        );

    ::blob blob =
        compress_state(
            ::gamePacket{
                .type = 0x09,
                .netid = pPeer->netid,
                .state = state::S_EXTENDED
            }
        );

    blob.u8(0x01);

    blob.push_back(
        pPeer->serialize_inventory()
    );

    ENetPacket *packet =
        enet_packet_create(
            blob.data().data(),
            blob.size(),
            ENET_PACKET_FLAG_RELIABLE
        );

    if (
        enet_peer_send(
            event.peer,
            0,
            packet
        )
    )
    {
        enet_packet_destroy(packet);
    }
}

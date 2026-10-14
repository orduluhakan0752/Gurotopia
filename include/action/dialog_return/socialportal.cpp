#include "pch.hpp"
#include "socialportal.hpp"
#include "action/friends.hpp"
#include "commands/__command.hpp"
#include "tools/create_dialog.hpp"

namespace
{
    void show_gtqs_notice(ENetEvent& event, const std::string& title, const std::string& message)
    {
        send_varlist(event.peer, {
            "OnDialogRequest",
            create_dialog()
                .set_default_color("`o")
                .add_label_with_icon("big", title, 1366)
                .add_spacer("small")
                .add_textbox(message)
                .add_spacer("small")
                .add_button("gtqs_back", "`5Back to Network Hub``")
                .add_quick_exit()
                .end_dialog("socialportal", "Close")
        });
    }

    bool run_native_command(ENetEvent& event, std::string_view command)
    {
        const auto it = cmd_pool.find(command);
        if (it == cmd_pool.end())
            return false;
        it->second(event, command);
        return true;
    }
}

void socialportal(ENetEvent& event, const ::hPipe& hPipe)
{
    const std::string button = hPipe["buttonClicked"];

    if (button == "gtqs_back" || button == "back")
    {
        const std::string empty{};
        action::friends(event, empty);
        return;
    }

    if (button == "showfriend" || button == "gtqs_friends")
    {
        send_varlist(event.peer, {
            "OnDialogRequest",
            "set_default_color|`o\n"
            "add_label_with_icon|big|`wGTQS Friends``|left|1366|\n"
            "add_spacer|small|\n"
            "add_textbox|`oFriend management is handled by the game client.``|left|\n"
            "add_button|gtqs_back|Back to Network Hub|noflags|0|0|\n"
            "add_quick_exit|\n"
            "end_dialog|socialportal||\n"
        });
        return;
    }

    if (button == "gtqs_btc")
    {
        if (!run_native_command(event, "btc"))
            show_gtqs_notice(event, "`6BTC MARKET``", "`oBTC Market is not registered in this C++ command pool. Keep the existing GTQS Lua BTC module enabled and connect its command handler to this button.``");
        return;
    }

    if (button == "gtqs_opc")
    {
        if (!run_native_command(event, "opc"))
            show_gtqs_notice(event, "`cOPC HUB``", "`oOPC Hub is not registered in this C++ command pool. Keep the existing GTQS Lua OPC module enabled and connect its command handler to this button.``");
        return;
    }

    struct NativeShortcut { const char* id; const char* command; const char* title; };
    static constexpr NativeShortcut shortcuts[] = {
        {"gtqs_guild", "guild", "`wGUILD``"},
        {"gtqs_events", "event", "`wEVENTS``"},
        {"gtqs_rewards", "daily", "`2REWARDS``"},
        {"gtqs_pets", "pets", "`wPETS``"},
        {"gtqs_staff", "ranks", "`5STAFF & RANKS``"},
        {"gtqs_settings", "settings", "`wSETTINGS``"},
        {"trade_history", "tradehistory", "`wTRADE HISTORY``"}
    };

    for (const auto& shortcut : shortcuts)
    {
        if (button == shortcut.id)
        {
            if (!run_native_command(event, shortcut.command))
                show_gtqs_notice(event, shortcut.title,
                    "`oThis feature is not registered as a native C++ command in this source build. The existing feature module must be connected here before this button can open it.``");
            return;
        }
    }

    // Preserve the old friend-dialog actions when clients still send them.
    if (button == "friend_all" || button == "all_friends" || button == "friends_options")
    {
        show_gtqs_notice(event, "`wGTQS FRIENDS``", "`oThis action is handled by the client-side friends interface.``");
        return;
    }
}

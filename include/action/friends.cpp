#include "pch.hpp"
#include "tools/create_dialog.hpp"
#include "friends.hpp"

// GTQS Network Hub: replaces the default Social Portal landing dialog.
// The game dialog renderer supports text/colors/buttons, not arbitrary PNG UI.
void action::friends(ENetEvent& event, const std::string& header)
{
    send_varlist(event.peer, {
        "OnDialogRequest",
        create_dialog()
            .set_default_color("`o")
            .add_label_with_icon("big", "`wGTQS `5NETWORK HUB``", 1366)
            .add_smalltext("`5CYBER GAMING  `w|  Play  •  Trade  •  Earn  •  Grow``")
            .add_spacer("small")
            .add_textbox("`wChoose a GTQS service below.``")
            .add_spacer("small")
            .add_button("gtqs_friends", "`wFriends``")
            .add_button("gtqs_guild", "`wGuild``")
            .add_button("gtqs_events", "`wEvents``")
            .add_spacer("small")
            .add_button("gtqs_btc", "`6BTC Market``")
            .add_button("gtqs_opc", "`cOPC Hub``")
            .add_button("gtqs_rewards", "`2Rewards``")
            .add_spacer("small")
            .add_button("gtqs_pets", "`wPets``")
            .add_button("gtqs_staff", "`5Staff & Ranks``")
            .add_button("gtqs_settings", "`wSettings``")
            .add_spacer("small")
            .add_textbox("`5GTQS NETWORK  `w|  Play Together  |  Be a Legend``")
            .add_quick_exit()
            .end_dialog("socialportal", "Continue")
    });
}

#include "client.h"
#include "game.h"
#include "log.h"
#include "player.h"

using namespace rj;
using namespace rj::net;

connection::connection(game *game, SOCKET sock, const sockaddr_storage &addr) : buffered_socket(sock, addr), game_(game)
{
}

connection::connection(game *game) : buffered_socket(), game_(game)
{
}

connection::connection(connection &&other) : buffered_socket(std::move(other)), game_(other.game_)
{
}

connection::~connection()
{
    log_trace("destrying connection");
}

connection &connection::operator=(connection &&other)
{
    buffered_socket::operator=(std::move(other));

    game_ = other.game_;

    return *this;
}

//! handle when connected
void connection::on_connect()
{
    json packet;

    // build the init packet
    packet["action"] = CONNECTION_INIT;

    std::vector<json> players;

    game_->for_players([&](const shared_ptr<player> &p) {
        if (p->get_connection() != this) {
            players.push_back(p->to_packet());
        }
        return false;
    });

    packet["players"] = players;

    log_trace("connection connected, sending %s", packet.to_string().c_str());

    writeln(packet);

    log_trace("connection connected, sending %s", packet.to_string().c_str());
}

//! handle when closed
void connection::on_close()
{
    log_trace("connection closed (%s: %s)", errno, strerror(errno));

    game_->action_remove_network_player(this);
}

void connection::on_will_read()
{
    // logstr("connection will read");
}

//! handle when data is read
void connection::on_did_read()
{
    json packet;

    while (has_input()) {
        // read a packet
        if (!packet.parse(readln())) {
            log_trace("unable to parse line from buffer");
            return;
        }

        log_trace("recieved %s", packet.to_string().c_str());

        // validate the packet
        // TODO: verify a game/session id?
        if (packet.find("action") == packet.end()) {
            throw runtime_error("packet has no action");
        }

        client_action action = static_cast<client_action>(packet["action"].get<int>());

        switch (action) {
            case REMOTE_CONNECTION_INIT: {
                log_trace("handling remote connect init");
                handle_remote_connection_init(packet);
                break;
            }
            case PLAYER_JOINED: {
                log_trace("handling player joined");
                handle_player_joined(packet);
                break;
            }
            case PLAYER_LEFT: {
                log_trace("handling player left");
                handle_player_left(packet);
                break;
            }
            case PLAYER_ROLL: {
                log_trace("handling player roll");
                handle_player_roll(packet);
                break;
            }
            /* connection is someone connect to the server */
            case CONNECTION_INIT: {
                log_trace("handling connection init");
                handle_connection_init(packet);
                break;
            }
            case GAME_START: {
                log_trace("handling game start");
                handle_game_start(packet);
                break;
            }
            case PLAYER_TURN_FINISHED: {
                log_trace("handling player turn finished");
                handle_player_turn_finished(packet);
                break;
            }
            default: {
                log_trace("unknown action for packet");
                break;
            }
        }
    }
}

void connection::on_will_write()
{
    log_trace("connection will write");
}

void connection::on_did_write()
{
    log_trace("connection did write");
}

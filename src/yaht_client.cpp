#include "yaht_client.h"
#include <arg3dice/yaht/engine.h>
#include <arg3json/json.h>
#include "yaht_game.h"

using namespace arg3;
using namespace arg3::net;

yaht_connection::yaht_connection(yaht_game *game, SOCKET sock, const sockaddr_storage &addr) : buffered_socket(sock, addr), game_(game)
{}

yaht_connection::yaht_connection(yaht_game *game) : buffered_socket(), game_(game)
{
}

yaht_connection::yaht_connection(yaht_connection &&other) : buffered_socket(std::move(other)),
    game_(other.game_)
{}

yaht_connection::~yaht_connection()
{
}

yaht_connection &yaht_connection::operator=(yaht_connection && other)
{
    buffered_socket::operator=(std::move(other));

    game_ = other.game_;

    return *this;
}

void yaht_connection::on_connect()
{
    json::object packet;

    packet.set_int("action", CONNECTION_INIT);

    json::array names;

    for (auto & p : *yaht::engine::instance())
    {
        names.add_string(p.name());
    }

    packet.set_array("players", names);

    writeln(packet.to_string());
}

void yaht_connection::on_close()
{
}

void yaht_connection::on_will_read()
{
}

void yaht_connection::on_did_read()
{
    json::object packet;

    packet.parse(readln());

    client_action action = (client_action) packet.get_int("action");

    switch (action)
    {
        /* client is the person joining */
    case CLIENT_INIT:
    {
        string name = packet.get_string("name");

        game_->action_add_network_player(name);

        break;
    }
    /* connection is someone connect to the server */
    case CONNECTION_INIT:
    {
        json::array names = packet.get_array("players");

        for (auto & name : names)
        {
            yaht::engine::instance()->add_player(name);
        }

        game_->action_joined_game();

        break;
    }
    }
}

void yaht_connection::on_will_write()
{
}

void yaht_connection::on_did_write()
{
}

yaht_connection_factory::yaht_connection_factory(yaht_game *game) : game_(game)
{
}

std::shared_ptr<buffered_socket> yaht_connection_factory::create_socket(socket_server *server, SOCKET sock, const sockaddr_storage &addr)
{
    auto socket = make_shared<yaht_connection>(game_, sock, addr);

    return socket;
}



yaht_client::yaht_client(yaht_game *game, SOCKET sock, const sockaddr_storage &addr) : yaht_connection(game, sock, addr), backgroundThread_(nullptr)
{}

yaht_client::yaht_client(yaht_game *game) : yaht_connection(game), backgroundThread_(nullptr)
{
}

yaht_client::yaht_client(yaht_client &&other) : yaht_connection(std::move(other)), backgroundThread_(std::move(other.backgroundThread_))
{}

yaht_client::~yaht_client()
{
    if (backgroundThread_ != nullptr)
    {
        backgroundThread_->join();
        backgroundThread_ = nullptr;
    }
}

yaht_client &yaht_client::operator=(yaht_client && other)
{
    yaht_connection::operator=(std::move(other));

    backgroundThread_ = std::move(other.backgroundThread_);

    return *this;
}

void yaht_client::on_connect()
{
    json::object packet;

    packet.set_int("action", CLIENT_INIT);
    packet.set_string("name", (*yaht::engine::instance())[0]->name());

    writeln(packet.to_string());
}


bool yaht_client::start(const std::string &host, int port)
{
    if (!connect(host, port))
        return false;

    run();
    return true;
}

bool yaht_client::start_in_background(const std::string &host, int port)
{
    if (!connect(host, port))
        return false;

    set_non_blocking(true);

    backgroundThread_ = make_shared<thread>(&yaht_client::run, this);
    return true;
}

void yaht_client::run()
{
    std::chrono::milliseconds dura( 100 );
    while (is_valid())
    {
        if (!read_to_buffer())
        {
            std::this_thread::sleep_for( dura );
        }

        if (!write_from_buffer())
        {
            std::this_thread::sleep_for( dura );
        }

        std::this_thread::sleep_for( dura );
    }
}

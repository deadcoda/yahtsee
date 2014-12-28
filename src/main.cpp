
#include <stdlib.h>

#include "caca.h"

#include "game.h"

using namespace std;

using namespace arg3;

int main(int argc, char **argv)
{
    ::game game;

    srand(time(0));

    game.load_settings();

    game.start();

    while (game.alive())
    {
        game.update();

        usleep(50);
    }

    game.reset();

    return 0;
}


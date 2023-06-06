enum {
    // types of packets
    SPAWN,
    LIST,
    ATTACH,
    KILL,

    // signals to server
    SAVE_SESSION = -1,
    KILL_SESSION = -2,
    END_OF_INPUT = 0,

    BUFFER_SIZE = 1000
};

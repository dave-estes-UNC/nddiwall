Overview
========

The sync feature provides a basic synchronization scheme to the NDDI Link. Such a scheme isn't necessary
when the driving application is very near the display. However, when the NDDI Link is implemented by
GRPC, for instance, network latencies and bandwidth make even 24 fps video difficult to render
consistently.

Additionally, the sync feature will allow multiple applications to collaborate to build frames of scenes
at a regular refresh rate.


API Updates
===========

    void RegisterClient()
    -   Each client must register with the NDDI Display using this API. The client will receive a unique
        client ID to be uses for future interactions.

    void DeregisterClient()
    -   Deregisters the client.

    bool ReserveDisplayRegion(ClientID id, vector<unsigned int> start, vector<unsigned int> end)
    -   Reserves a region of the display for the client. This prevents any other client from updating
        the CoefficientPlanes within this region.
    -   Returns true on success and false on failure.

    vector<Reservation> GetDisplayRegionReservations()
    -   Returns the current reserved regions of the display along with the ClientIDs.

    bool ReserveFrameVolumeRegion(ClientID id, vector<unsigned int> start, vector<unsigned int> end)
    -   Reserves a region of the Frame VOlume for a client. This prevents any other client from updating
        the CoefficientPlanes within this region.
    -   Returns true on success and false on failure.

    vector<Reservation>  GetFrameVolumeRegionReservations()
    -   Returns the current reserved regions of the FrameVolume along with the ClientIDs.

    uint64_t GetTime()
    -   Returns the current millisecond time tick since the creation of the display.

    void Latch()
    -   Causes an immediate latch of all of the registered regions for the given client.

    int64_t Latch(uint64_t time)
    -   Schedules a latch for the given client at the given time.
    -   Forms a barrier for any of this client's commands. Any commands before the latch are executed.
        Any commands after the latch will remain buffered until the latch executes.
    -   If a latch is received before it's scheduled time, then the positive difference of those times
        is returned.
    -   If a latch is received after it's scheduled time, then it is immediately processed and the
        negative difference of those times is returned.

    Miscellaneous
    -   All NDDI commands are tagged with the client's unique idea assigned when registering. I'll try
        to make this transparent. Otherwise it will be be passed explicitly.

Use Cases
=========

Streaming Video
---------------

-   The client will use the scheduled latch to try and buffer X seconds of video.
-   The client will use the time difference returned when scheduling a latch to determine the best
    "bit rate" to use when encoding.


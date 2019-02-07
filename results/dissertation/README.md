Overview
--------

The following illustrates how to record the nDDI commands, play them back, and
then how to roll the results up. The source video and recorded commands are
archived under the Research folder on Google Drive (cdesets@cs.unc.edu).

Recording
---------

vidcon

    $ ./nddiwall_pixelbridge_client --mode dct --start 60 --frames 240 --subregion 0 0 1920 1080 ~/Work/recordings/dissertation/vidcon/vidcon1-1080p.mov --record ~/Work/recordings/dissertation/vidcon/cmds/vidcon1-1080p.cmd
    $ ./nddiwall_pixelbridge_client --mode dct --start 60 --frames 240 --subregion 1920 0 1920 1080 ~/Work/recordings/dissertation/vidcon/vidcon2-1080p.mov --record ~/Work/recordings/dissertation/vidcon/cmds/vidcon2-1080p.cmd
    $ ./nddiwall_pixelbridge_client --mode dct --start 60 --frames 240 --subregion 3840 0 1920 1080 ~/Work/recordings/dissertation/vidcon/vidcon3-1080p.mov --record ~/Work/recordings/dissertation/vidcon/cmds/vidcon3-1080p.cmd
    $ ./nddiwall_pixelbridge_client --mode dct --start 60 --frames 240 --subregion 5760 0 1920 1080 ~/Work/recordings/dissertation/vidcon/vidcon4-1080p.mov --record ~/Work/recordings/dissertation/vidcon/cmds/vidcon4-1080p.cmd

    $ ./nddiwall_pixelbridge_client --mode dct --start 60 --frames 240 --scale 2 --subregion 0 1080 3840 2160 ~/Work/recordings/dissertation/vidcon/vidcon0-1080p.mov --record ~/Work/recordings/dissertation/vidcon/cmds/vidcon0-1080p.cmd
    $ ./nddiwall_pixelbridge_client --mode dct --start 60 --frames 240 --scale 2 --subregion 3840 1080 3840 2160 ~/Work/recordings/dissertation/vidcon/presentation-1080p.mov --record ~/Work/recordings/dissertation/vidcon/cmds/presentation-1080p.cmd

    $ ./nddiwall_pixelbridge_client --mode dct --start 60 --frames 240 --subregion 0 3240 1920 1080 ~/Work/recordings/dissertation/vidcon/vidcon5-1080p.mov --record ~/Work/recordings/dissertation/vidcon/cmds/vidcon5-1080p.cmd
    $ ./nddiwall_pixelbridge_client --mode dct --start 60 --frames 240 --subregion 1920 3240 1920 1080 ~/Work/recordings/dissertation/vidcon/vidcon6-1080p.mov --record ~/Work/recordings/dissertation/vidcon/cmds/vidcon6-1080p.cmd
    $ ./nddiwall_pixelbridge_client --mode dct --start 60 --frames 240 --subregion 3840 3240 1920 1080 ~/Work/recordings/dissertation/vidcon/vidcon7-1080p.mov --record ~/Work/recordings/dissertation/vidcon/cmds/vidcon7-1080p.cmd
    $ ./nddiwall_pixelbridge_client --mode dct --start 60 --frames 240 --subregion 5760 3240 1920 1080 ~/Work/recordings/dissertation/vidcon/vidcon8-1080p.mov --record ~/Work/recordings/dissertation/vidcon/cmds/vidcon8-1080p.cmd

jumbotron

    $ ./nddiwall_pixelbridge_client --mode dct --start 480 --frames 240 --scale 2 --subregion 0 0 7680 4320 ~/Work/recordings/dissertation/jumbo/soccer-4k.mov --record ~/Work/recordings/dissertation/jumbo/cmds//soccer-4k.cmd
    $ ./nddiwall_pixelbridge_client --mode dct --start 60 --frames 240 --subregion 7680 0 1920 1080 ~/Work/recordings/dissertation/jumbo/commercial-1080p.ts --record ~/Work/recordings/dissertation/jumbo/cmds/commercial-1080p.cmd
    $ ./nddiwall_pixelbridge_client --mode dct --start 60 --frames 240 --subregion 7680 1080 1920 2160 ~/Work/recordings/dissertation/jumbo/scoreboard.m4v --record ~/Work/recordings/dissertation/jumbo/cmds/scoreboard.m4v.cmd
    $ ./nddiwall_pixelbridge_client --mode dct --start 60 --frames 240 --subregion 7680 3240 1920 1080 ~/Work/recordings/dissertation/jumbo/soccer-1080p.mov --record ~/Work/recordings/dissertation/jumbo/cmds/soccer-1080p.cmd
    $ ./nddiwall_pixelbridge_client --mode dct --start 60 --frames 240 --subregion 0 4320 9600 544 ~/Work/recordings/dissertation/jumbo/ticker.m4v --record ~/Work/recordings/dissertation/jumbo/cmds/ticker.cmd

billboard

    $ ./nddiwall_pixelbridge_client --mode dct --start 120 --frames 240 --subregion 0 0 1920 1080 ~/Work/recordings/dissertation/billboard/xmas-left.m4v --record ~/Work/recordings/dissertation/billboard/cmds/xmas-left.cmd
    $ ./nddiwall_pixelbridge_client --mode dct --start 120 --frames 240 --subregion 1920 0 1920 1080 ~/Work/recordings/dissertation/billboard/xmas-center.m4v --record ~/Work/recordings/dissertation/billboard/cmds/xmas-center.cmd
    $ ./nddiwall_pixelbridge_client --mode dct --start 120 --frames 240 --subregion 3840 0 1920 1080 ~/Work/recordings/dissertation/billboard/xmas-right.m4v --record ~/Work/recordings/dissertation/billboard/cmds/xmas-right.cmd
    $ ./nddiwall_pixelbridge_client --mode dct --start 120 --frames 240 --subregion 0 1080 2160 3840 ~/Work/recordings/dissertation/billboard/xmas-main.m4v --record ~/Work/recordings/dissertation/billboard/cmds/xmas-main.cmd
    $ ./nddiwall_pixelbridge_client --mode dct --start 120 --frames 240 --subregion 2160 1080 2160 960 ~/Work/recordings/dissertation/billboard/xmas-bottom.m4v --record ~/Work/recordings/dissertation/billboard/cmds/xmas-bottom.cmd
    $ ./nddiwall_pixelbridge_client --mode dct --start 240 --frames 240 --subregion 2160 2040 3840 2160 ~/Work/recordings/dissertation/billboard/skyfall-4k.mp4 --record ~/Work/recordings/dissertation/billboard/cmds/skyfall-4k.cmd


Running
-------

vidcon (live)

    $ ./nddiwall_server &
    $ ./nddiwall_master_client --display 7680 4320 --fv 24,16,64

    $ ./nddiwall_pixelbridge_client --mode dct --start 60 --frames 240 --subregion 0 0 1920 1080 ~/Work/recordings/dissertation/vidcon/vidcon1-1080p.mov &
    $ ./nddiwall_pixelbridge_client --mode dct --start 60 --frames 240 --subregion 1920 0 1920 1080 ~/Work/recordings/dissertation/vidcon/vidcon2-1080p.mov &
    $ ./nddiwall_pixelbridge_client --mode dct --start 60 --frames 240 --subregion 3840 0 1920 1080 ~/Work/recordings/dissertation/vidcon/vidcon3-1080p.mov &
    $ ./nddiwall_pixelbridge_client --mode dct --start 60 --frames 240 --subregion 5760 0 1920 1080 ~/Work/recordings/dissertation/vidcon/vidcon4-1080p.mov &

    $ ./nddiwall_pixelbridge_client --mode dct --start 60 --frames 240 --scale 2 --subregion 0 1080 3840 2160 ~/Work/recordings/dissertation/vidcon/vidcon0-1080p.mov &
    $ ./nddiwall_pixelbridge_client --mode dct --start 60 --frames 240 --scale 2 --subregion 3840 1080 3840 2160 ~/Work/recordings/dissertation/vidcon/presentation-1080p.mov &

    $ ./nddiwall_pixelbridge_client --mode dct --start 60 --frames 240 --subregion 0 3240 1920 1080 ~/Work/recordings/dissertation/vidcon/vidcon5-1080p.mov &
    $ ./nddiwall_pixelbridge_client --mode dct --start 60 --frames 240 --subregion 1920 3240 1920 1080 ~/Work/recordings/dissertation/vidcon/vidcon6-1080p.mov &
    $ ./nddiwall_pixelbridge_client --mode dct --start 60 --frames 240 --subregion 3840 3240 1920 1080 ~/Work/recordings/dissertation/vidcon/vidcon7-1080p.mov &
    $ ./nddiwall_pixelbridge_client --mode dct --start 60 --frames 240 --subregion 5760 3240 1920 1080 ~/Work/recordings/dissertation/vidcon/vidcon8-1080p.mov &

vidcon (recordings)

    $ ./nddiwall_server &
    $ ./nddiwall_master_client --display 7680 4320 --fv 24,16,64
    $ for x in `find ~/Work/recordings/dissertation/vidcon/cmds -name *.cmd`; do ./nddiwall_player_client $x; done

jumbotron (live)

    $ ./nddiwall_server &
    $ ./nddiwall_master_client --display 9600 4864 --fv 24,16,64

    $ ./nddiwall_pixelbridge_client --mode dct --start 480 --frames 240 --scale 2 --subregion 0 0 7680 4320 ~/Work/recordings/dissertation/jumbo/soccer-4k.mov &
    $ ./nddiwall_pixelbridge_client --mode dct --start 60 --frames 240 --subregion 7680 0 1920 1080 ~/Work/recordings/dissertation/jumbo/commercial-1080p.ts &
    $ ./nddiwall_pixelbridge_client --mode dct --start 60 --frames 240 --subregion 7680 1080 1920 2160 ~/Work/recordings/dissertation/jumbo/scoreboard.m4v &
    $ ./nddiwall_pixelbridge_client --mode dct --start 60 --frames 240 --subregion 7680 3240 1920 1080 ~/Work/recordings/dissertation/jumbo/soccer-1080p.mov &
    $ ./nddiwall_pixelbridge_client --mode dct --start 60 --frames 240 --subregion 0 4320 9600 544 ~/Work/recordings/dissertation/jumbo/ticker.m4v &

jumbotron (recordings)

    $ ./nddiwall_server &
    $ ./nddiwall_master_client --display 9600 4920 --fv 24,16,64
    $ for x in `find ~/Work/recordings/dissertation/jumbo/cmds -name *.cmd`; do ./nddiwall_player_client $x; done

billboard (live)

    $ ./nddiwall_server &
    $ ./nddiwall_master_client --display 6000 4920 --fv 24,16,64

    $ ./nddiwall_pixelbridge_client --mode dct --start 120 --frames 240 --subregion 0 0 1920 1080 ~/Work/recordings/dissertation/billboard/xmas-left.m4v
    $ ./nddiwall_pixelbridge_client --mode dct --start 120 --frames 240 --subregion 1920 0 1920 1080 ~/Work/recordings/dissertation/billboard/xmas-center.m4v
    $ ./nddiwall_pixelbridge_client --mode dct --start 120 --frames 240 --subregion 3840 0 1920 1080 ~/Work/recordings/dissertation/billboard/xmas-right.m4v
    $ ./nddiwall_pixelbridge_client --mode dct --start 120 --frames 240 --subregion 0 1080 2160 3840 ~/Work/recordings/dissertation/billboard/xmas-main.m4v
    $ ./nddiwall_pixelbridge_client --mode dct --start 120 --frames 240 --subregion 2160 1080 2160 960 ~/Work/recordings/dissertation/billboard/xmas-bottom.m4v
    $ ./nddiwall_pixelbridge_client --mode dct --start 240 --frames 240 --subregion 2160 2040 3840 2160 ~/Work/recordings/dissertation/billboard/skyfall-4k.mp4

billboard (recordings)

    $ ./nddiwall_server &
    $ ./nddiwall_master_client --display 6000 4920 --fv 24,16,64
    $ for x in `find ~/Work/recordings/dissertation/billboard/cmds -name *.cmd`; do ./nddiwall_player_client $x; done


Getting Counts
--------------

vidcon

    $ for x in `find ~/Work/recordings/dissertation/vidcon/ -name *1080p.mov`; do echo $x && ./nddiwall_pixelbridge_client --mode count --start 60 --frames 240 $x; done > ~/Work/recordings/dissertation/vidcon.counts

jumbotron

    $ for x in `echo -e "commercial-1080p.ts\nscoreboard.m4v\nsoccer-1080p.mov\nticker.m4v"`; do echo "/home/cdestes/Work/recordings/dissertation/jumbo/$x" && ./nddiwall_pixelbridge_client --mode count --start 60 --frames 240 ~/Work/recordings/dissertation/jumbo/$x; done > ~/Work/recordings/dissertation/jumbo.counts
    $ for x in `echo -e "soccer-4k.mov"`; do echo "/home/cdestes/Work/recordings/dissertation/jumbo/$x" && ./nddiwall_pixelbridge_client --mode count --start 480 --frames 240 ~/Work/recordings/dissertation/jumbo/$x; done >> ~/Work/recordings/dissertation/jumbo.counts

billboard

    $ for x in `find ~/Work/recordings/dissertation/billboard/ -name *.m4v`; do echo $x && ./nddiwall_pixelbridge_client --mode count --start 120 --frames 240 $x; done > ~/Work/recordings/dissertation/billboard.counts
    $ for x in `echo -e "skyfall-4k.mp4"`; do echo "/home/cdestes/Work/recordings/dissertation/billboard/$x" && ./nddiwall_pixelbridge_client --mode count --start 240 --frames 240 ~/Work/recordings/dissertation/billboard/$x; done >> ~/Work/recordings/dissertation/billboard.counts

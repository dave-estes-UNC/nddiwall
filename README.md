README
======

From this directory, you can build Release:

  mkdir release
  cd release
  cmake -DCMAKE_BUILD_TYPE=Release ..
  make -j8


or Debug:

  mkdir debug
  cd debug
  cmake -DCMAKE_BUILD_TYPE=Debug ..
  make -j8


Then to run the simple unit test (nddiwall_client):

  ./nddiwall_server &
  ./nddiwall_client


To run a proper pixelbridge client:

  ./nddiwall_server &
  ./pixelbridge <options> <path-to-video>
 

If you want to record commands with pixelbridge and playback later.

  ./pixelbridge <options> --record <record-filename> <path-to-video>
  ./nddiwall_server &
  ./nddiwall_player <record-filename>

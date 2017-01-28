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

Then to run.

  ./nddiwall_server &
  ./nddiwall_client

or

  ./nddiwall_server &
  ./pixelbridge <path-to-video>

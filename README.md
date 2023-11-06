[![License: GPL-2.0-or-later](https://img.shields.io/badge/License-GPL%20v2+-blue.svg)](LICENSE.md)
[![Build Status](https://dev.azure.com/teamkodi/kodi-pvr/_apis/build/status/kodi-pvr.pvr.dvblink?branchName=Nexus)](https://dev.azure.com/teamkodi/kodi-pvr/_build/latest?definitionId=59&branchName=Nexus)
[![Build Status](https://jenkins.kodi.tv/view/Addons/job/kodi-pvr/job/pvr.dvblink/job/Nexus/badge/icon)](https://jenkins.kodi.tv/blue/organizations/jenkins/kodi-pvr%2Fpvr.dvblink/branches/)

# DVBLink PVR
DVBLink PVR client addon for [Kodi](https://kodi.tv)

## Build instructions

### Linux

1. `git clone --branch master https://github.com/xbmc/xbmc.git`
2. `git clone https://github.com/kodi-pvr/pvr.dvblink.git`
3. `cd pvr.dvblink && mkdir build && cd build`
4. `cmake -DADDONS_TO_BUILD=pvr.dvblink -DADDON_SRC_PREFIX=../.. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=../../xbmc/addons -DPACKAGE_ZIP=1 ../../xbmc/cmake/addons`
5. `make`

##### Useful links

* [Kodi's PVR user support](https://forum.kodi.tv/forumdisplay.php?fid=167)
* [Kodi's PVR development support](https://forum.kodi.tv/forumdisplay.php?fid=136)

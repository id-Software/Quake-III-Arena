#!/bin/sh
rmdir --ignore-fail-on-non-empty demoq3 missionpack >& /dev/null
if test -e "$SETUP_INSTALLPATH"/ioquake3.desktop.in; then
  xdg_desktop_menu=`which xdg-desktop-menu 2>/dev/null`
  if test "x$xdg_desktop_menu" = x; then
    xdg_desktop_menu=./xdg-desktop-menu
  fi
  $xdg_desktop_menu uninstall --novendor ioquake3.desktop
  rm ioquake3.desktop
fi

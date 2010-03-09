<?php
/*
 Sets the default host that will be seen on the login page
*/
define('DEFAULT_HOST', '127.0.0.1');

/*
 Sets the default port that will be seen on the login page
*/
define('DEFAULT_PORT', '56789');

/*
 Sets the template/theme that should be used for displaying. Right now there is only 'default'
*/
define('TEMPLATE', 'default');

/*
 Sets the language that should be used. Right now there is 'en' and 'de', but not all parts will be
 translated yet, if you set something else than 'en'. But feel free to try it out.
*/
define('LANG', 'en');

/*
 If this is set to true, ddclient-php will automatically refresh the displayed download list if
 there are running downloads. If no download is running, the page will not reload, so it doesn't
 prevent the machine where DownloadDaemon runs on from spinning down the hard-disk.
*/
define('AUTO_REFRESH', true);
?>

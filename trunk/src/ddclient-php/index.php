<?php
include 'inc/_config.inc.php';
include 'inc/functions.inc.php';
include 'inc/template.inc.php';

//Language File
if(file_exists('lang/'.LANG.'.php')){
	include 'lang/'.LANG.'.php';	
}else{
	include 'lang/en.php';	
}

//Logged in?
if(isset($_COOKIE['ddclient_host'])){
	$logged_in = true;	
}else{
	$logged_in = false;
}


if(isset($_GET['site']) && file_exists('sites/'.$_GET['site'].'.php') && $logged_in == true){
	$site = $_GET['site'];
}else{
	$site = 'login';
}

$site_url = "";
if(!isset($_SERVER['HTTPS']) || strtolower($_SERVER['HTTPS']) == "off") {
	$site_url = "http://";
} else {
	$site_url = "https://";
}

$tpl_vars = array(
	'T_SITE_URL' => $site_url . $_SERVER['HTTP_HOST'] . substr($_SERVER['SCRIPT_NAME'], 0, strrpos($_SERVER['SCRIPT_NAME'], "/")+1),
	'T_DEFAULT_LANG' => LANG,
	'L_DD' => $LANG['DD'],
	'L_Manager' => $LANG['Manager'],
	'L_Add' => $LANG['Add'],
	'L_List' => $LANG['List'],
	'L_Manage' => $LANG['Manage'],
	'L_Config' => $LANG['Config'],
	'L_Logout' => $LANG['Logout'],
	'L_Site' => $LANG[ucfirst($site)],
	'T_META' => '',
);

include 'sites/'.$site.'.php';

echo template_parse_site($site, $tpl_vars);
?>

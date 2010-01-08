<?php
include 'inc/_config.inc.php';
include 'inc/functions.inc.php';
include 'inc/template.inc.php';
include 'lang/en.php';

if(isset($_COOKIE['ddclient_host'])){
	$logged_in = true;	
}else{
	$logged_in = false;
//	$logged_in = true;
}


if(isset($_GET['site']) && file_exists('sites/'.$_GET['site'].'.php') && $logged_in == true){
	$site = $_GET['site'];
}else{
	$site = 'login';
}

include 'sites/'.$site.'.php';
echo parse_template($site, $tpl_vars);
?>

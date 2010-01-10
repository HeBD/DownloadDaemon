<?php
$err_message = '';
if(isset($_POST['submit'])) {
	$socket = socket_create(AF_INET, SOCK_STREAM, 0);
	$day = 86400 + time();
	$host = $_POST['host'];
	$port = $_POST['port'];
	$pwd = $_POST['passwd'];
	if($pwd == "") {
		$pwd = " ";
	}
	if(isset($_POST['encrypt'])) {
		$enc = $_PORT['encrypt'];
	}else{
		$enc = 0;
	}
	$timeout = 5;
	
	if(!preg_match('/^\d+\.\d+\.\d+\.\d+$/', $host)) {
		$host = gethostbyname($host);
	}
	if(!is_numeric($port)) {
		$err_message = msg_generate($LANG['ERR_INVALID_PORT'], 'error');
	} else {

		$test = connect_to_daemon($socket, $host, $port, $pwd, $enc, $timeout);
	
		if($test == 'SUCCESS') {
			setcookie("ddclient_host", $host, $day);
			setcookie("ddclient_port", $port, $day);
			setcookie("ddclient_passwd", $pwd, $day);
			setcookie("ddclient_enc", $enc, $day);
			header("Location: index.php?site=manage");
		} else {
			$err_message = msg_generate($LANG[$test], 'error');	
		}
	}
}


$tpl_vars['L_Login'] = $LANG['Login'];
$tpl_vars['L_Host'] = $LANG['Host'];
$tpl_vars['L_Port'] = $LANG['Port'];
$tpl_vars['L_Password'] = $LANG['Password'];
$tpl_vars['L_Encrypt'] = $LANG['Encrypt'];
$tpl_vars['C_DEFAULT_HOST'] = DEFAULT_HOST;
$tpl_vars['C_DEFAULT_PORT'] = DEFAULT_PORT;
$tpl_vars['err_message'] = $err_message;
?>
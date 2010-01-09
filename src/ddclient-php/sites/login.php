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
			header("Location: index.php?site=list");
		} else {
			$err_message = msg_generate($LANG[$test], 'error');	
		}
	}
} else {

	$tpl_vars = array('L_DD' => $LANG['DD'],
		'L_Add_DL' => $LANG['Add_DL'],
		'L_List_DL' => $LANG['List_DL'],
		'L_Manage_DL' => $LANG['Manage_DL'],
		'L_Config_DD' => $LANG['Config_DD'],
		'L_Logout' => $LANG['Logout'],
		'L_Site' => $LANG['Login'],
		'L_Host' => $LANG['Host'],
		'L_Port' => $LANG['Port'],
		'L_Password' => $LANG['Password'],
		'L_Encrypt' => $LANG['Encrypt'],
		'C_DEFAULT_HOST' => DEFAULT_HOST,
		'C_DEFAULT_PORT' => DEFAULT_PORT,
		'err_message' => $err_message,
	);
}
?>

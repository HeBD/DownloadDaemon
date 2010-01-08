<?php
$err_message = '';
$content = '';
$socket = socket_create(AF_INET, SOCK_STREAM, 0);
connect_to_daemon($socket, $_COOKIE['ddclient_host'], $_COOKIE['ddclient_port'], $_COOKIE['ddclient_passwd'], $_COOKIE['ddclient_enc'], 5);
	if(isset($_POST['apply'])) {
		$buf = "";
		send_all($socket, "DDP PREMIUM SET " . $_POST['host'] . " " . $_POST['user'] . ";" . $_POST['pass']);
		recv_all($socket, $buf);
		if($buf == "100 SUCCESS") {
			$content .= "Successfully set premium credentials for " . $_POST['host'];
		} else {
			$content .= "failed to set premium credentials for " . $_POST['host'];
		}
	}

	$buf = "";
	send_all($socket, "DDP PREMIUM LIST");
	recv_all($socket, $buf);
	$host_list = explode("\n", $buf);
	$user_list = "";

	$content .= "Host: ";
	$content .= "<select name=\"host\">";
	$content .= "<option value=\"\"></option>";
	for($i = 0; $i != count($host_list); $i++) {
		$content .= "<option value=\"" . $host_list[$i] . "\"";
		$content .= ">" . $host_list[$i] . "</option>";
	}
	$content .= "</select>";
	$content .= "<br><br>";
	$content .= "Username: <input type=\"text\" name=\"user\"><br>";
	$content .= "Password: <input type=\"password\" name=\"pass\"><br>";
	$content .= "<input type=\"submit\" name=\"apply\" value=\"Apply\" id=\"submit\">";	

$tpl_vars = array('L_DD' => $LANG['DD'],
	'L_Add_DL' => $LANG['Add_DL'],
	'L_List_DL' => $LANG['List_DL'],
	'L_Manage_DL' => $LANG['Manage_DL'],
	'L_Config_DD' => $LANG['Config_DD'],
	'L_Logout' => $LANG['Logout'],
	'L_Site' => $LANG['Config_DD'],
	'content' => $content,
	'err_message' => $err_message,
);

?>

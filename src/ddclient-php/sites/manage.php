<?php
$err_message = '';
$dl_list = '';
$socket = socket_create(AF_INET, SOCK_STREAM, 0);
connect_to_daemon($socket, $_COOKIE['ddclient_host'], $_COOKIE['ddclient_port'], $_COOKIE['ddclient_passwd'], $_COOKIE['ddclient_enc'], 5);

if(isset($_POST['activate'])) {
	send_all($socket, "DDP DL ACTIVATE " . $_POST['id']);
	$buf = "";
	recv_all($socket, $buf);
	if(mb_substr($buf, 0, 3) != "100") {
		$err_message .= "Could not activate download " . $_POST['id'];
	}
}

if(isset($_POST['deactivate'])) {
	send_all($socket, "DDP DL DEACTIVATE " . $_POST['id']);
	$buf = "";
	recv_all($socket, $buf);
	if(mb_substr($buf, 0, 3) != "100") {
		$err_message .= "Could not deactivate download " . $_POST['id'];
	}
}

if(isset($_POST['delete'])) {
	send_all($socket, "DDP DL DEL " . $_POST['id']);
	$buf = "";
	recv_all($socket, $buf);
	if(mb_substr($buf, 0, 3) != "100") {
		$err_message .= "Could not delete download " . $_POST['id'];
	}
}

if(isset($_POST['delete_file'])) {
	send_all($socket, "DDP FILE DEL " . $_POST['id']);
	$buf = "";
	recv_all($socket, $buf);
	if(mb_substr($buf, 0, 3) != "100") {
		$err_message .= "Could not delete File " . $_POST['id'];
	}
}

if(isset($_POST['up'])) {
	send_all($socket, "DDP DL UP " . $_POST['id']);
	$buf = "";
	recv_all($socket, $buf);
	if(mb_substr($buf, 0, 3) != "100") {
		$err_message .= "Could not move download upwards";
	}
}

$list = "";
send_all($socket, "DDP DL LIST");
recv_all($socket, $list);

//$list = "0|Sun Jan  3 02:07:08 2010|Test Download #1|http://rapidshare.com/asdf|DOWNLOAD_INACTIVE|34627|34627|0|PLUGIN_SUCCESS|0\n";

$download_count = substr_count($list, "\n");
$download_index[] = array();
$download_index = explode("\n", $list);

$exp_dls[] = array();
for($i = 0; $i < $download_count; $i++) {
	$exp_dls[$i] = explode ( '|'  , $download_index[$i] );
}

for($i = 0; $i < $download_count; $i++) {
	$dl_list .= '<tr><td>'.$exp_dls[$i][0].'</td><td>'.$exp_dls[$i][1].'</td><td>'.$exp_dls[$i][2].'</td><td>'.$exp_dls[$i][3].'</td><td>';
	$dl_list .= '<form action="index.php?site=manage" method="post">';
	if($exp_dls[$i][4] == "DOWNLOAD_INACTIVE") {
		$dl_list .= '<input type="submit" name="activate" value=" " id="activate" />';
	} else {
		$dl_list .= '<input type="submit" name="deactivate" value=" " id="deactivate" />';
	}
	$dl_list .= '<input type="submit" name="delete" value=" " id="delete" />';
	$dl_list .= '<input type="submit" name="up" value=" " id="up" />';

	send_all($socket, "DDP FILE GETPATH " . $exp_dls[$i][0]);
	$buf = "";
	recv_all($socket, $buf);
	if($buf != "") {
		$dl_list .= '<input type="submit" name="delete_file" value="Delete File" id="delete" />';
	}

	$dl_list .= "<input type=\"hidden\" name=\"id\" value=\"" . $exp_dls[$i][0] . "\">";
	$dl_list .= "</form>";
	$dl_list .= "</td>";
	$dl_list .= "</tr>";
}

$dl_list .= "</table>";

$tpl_vars = array('L_DD' => $LANG['DD'],
	'L_Add_DL' => $LANG['Add_DL'],
	'L_List_DL' => $LANG['List_DL'],
	'L_Manage_DL' => $LANG['Manage_DL'],
	'L_Config_DD' => $LANG['Config_DD'],
	'L_Logout' => $LANG['Logout'],
	'L_Site' => $LANG['List_DL'],
	'L_Title' => $LANG['Title'],
	'L_URL' => $LANG['URL'],
	'L_ID' => $LANG['ID'],
	'L_Date' => $LANG['Date'],
	'L_Status' => $LANG['Status'],
	'T_List' => $dl_list,
	'err_message' => $err_message,
);
?>

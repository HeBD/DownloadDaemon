<?php
$err_message = '';
if(isset($_POST['submit_single']) || isset($_POST['submit_multiple'])) {
	$socket = socket_create(AF_INET, SOCK_STREAM, 0);
	connect_to_daemon($socket, $_COOKIE['ddclient_host'], $_COOKIE['ddclient_port'], $_COOKIE['ddclient_passwd'], $_COOKIE['ddclient_enc'], 5);
	
	if(isset($_POST['submit_single'])) {
		if($_POST['url'] != '') {
			send_all($socket, "DDP DL ADD " . $_POST['url'] . " " . $_POST['title']);
			$recv = "";
			recv_all($socket, $recv);
			if($recv[0] == "-") {
				$err_message .= msg_generate($LANG['ERR_URL_INVALID'], 'error');
			} else {
				$err_message .= msg_generate($LANG['SUCC_ADD_SINGLE'], 'success');
			}
		}
	} elseif(isset($_POST['submit_multiple'])) {
		$list = $_POST['titles_urls'];
		$download_count = substr_count($list, "http://");
		if($download_count == 0) {
			$download_count = substr_count($list, "ftp://");
		}
		if($download_count == 0) {
			echo "No valid URL entered.";
		}
	
		$list = str_replace("|", " ", $list);
		$download_index[] = array();
		$download_index = explode("\n", $list);
		$all_success = true;
		for($i = 0; $i < $download_count; $i++) {
			$buf = "";
			send_all($socket, "DDP DL ADD " . $download_index[$i]);
			recv_all($socket, $buf);
	
			if(mb_substr($buf, 0, 3) != "100") {
				echo "Error adding download: " . $download_index[$i] . ": URL is probably invalid.";
				$all_success = false;
			}
		}
		if($all_success) {
			$err_message .= msg_generate($LANG['SUCC_ADD_MULTI'], 'success');
		}
	}
}

$tpl_vars = array('L_DD' => $LANG['DD'],
	'L_Add_DL' => $LANG['Add_DL'],
	'L_List_DL' => $LANG['List_DL'],
	'L_Manage_DL' => $LANG['Manage_DL'],
	'L_Config_DD' => $LANG['Config_DD'],
	'L_Logout' => $LANG['Logout'],
	'L_Site' => $LANG['Add_DL'],
	'L_Title' => $LANG['Title'],
	'L_URL' => $LANG['URL'],
	'L_Add_single_DL' => $LANG['Add_single_DL'],
	'L_Add_multi_DL' => $LANG['Add_multi_DL'],
	'L_Add_multi_DL_Desc' => $LANG['Add_multi_DL_Desc'],
	'err_message' => $err_message,
);
?>

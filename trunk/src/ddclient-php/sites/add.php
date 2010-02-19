<?php
// Connect to Daemon
$socket = socket_create(AF_INET, SOCK_STREAM, 0);
$connect = connect_to_daemon($socket, $_COOKIE['ddclient_host'], $_COOKIE['ddclient_port'], $_COOKIE['ddclient_passwd'], $_COOKIE['ddclient_enc'], 5);

// Site Vars
$err_message = '';
$dl_list = '';
$content = '';
$list = "";

if($connect != 'SUCCESS') {
	$err_msg = msg_generate($LANG[$connect], 'error');
} else {
	send_all($socket, "DDP DL LIST");
	recv_all($socket, $list);

}

if($connect != 'SUCCESS') {
} elseif(isset($_POST['submit_single'])) {
	if($_POST['url'] != '') {
		$pkg_id = $_POST['pkg_single_sel'];
		if($pkg_id == "-1") {
			send_all($socket, "DDP PKG ADD " . $_POST['pkg_single']);
			recv_all($socket, $pkg_id);
		}

		if($pkg_id == -1) {
			$err_message .= msg_generate($LANG['ERR_PKG_OR_DL_NAM'], 'error');
		} else {
			send_all($socket, "DDP DL ADD " . $pkg_id . " " . $_POST['url'] . " " . $_POST['title']);
			$recv = "";
			recv_all($socket, $recv);
			if(mb_substr($recv, 0, 3) != "100") {
				$err_message .= msg_generate($LANG['ERR_URL_INVALID'], 'error');
			} else {
				$err_message .= msg_generate($LANG['SUCC_ADD_SINGLE'], 'success');
			}
		}
	}
} elseif(isset($_POST['submit_multi'])) {
	$urls = $_POST['titles_urls'];
	$urls = str_replace("|", " ", $urls);
	$download_index[] = array();
	$download_index = explode("\n", $urls);
	$all_success = true;
	$pkg_id = $_POST['pkg_multi_sel'];
	if($pkg_id == "-1") {
		send_all($socket, "DDP PKG ADD " . $_POST['pkg_multi']);
		recv_all($socket, $pkg_id);
	}
	if($pkg_id == -1) {
		$err_message .= msg_generate($LANG['ERR_PKG_OR_DL_NAM'], 'error');
	} else {
		for($i = 0; $i < count($download_index); $i++) {
			if(strpos($download_index[$i], "http://") === FALSE && strpos($download_index[$i], "ftp://") === FALSE) {
				continue;
			}
			$buf = "";
			send_all($socket, "DDP DL ADD " . $pkg_id . " " . $download_index[$i]);
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

$download_index[] = array();
$download_index = explode("\n", $list);
for($i = 0; $i < count($download_index); $i++) {
	if(strpos($download_index[$i], "PACKAGE|") === 0) {
		$pkg = explode("|", $download_index[$i]);
		$content .= "<option value=\"". $pkg[1] . "\">" . $pkg[2] . "</option>";
	}
}




$tpl_vars['L_Title'] = $LANG['Title'];
$tpl_vars['L_URL'] = $LANG['URL'];
$tpl_vars['L_Add_single_DL'] = $LANG['Add_single_DL'];
$tpl_vars['L_Add_multi_DL'] = $LANG['Add_multi_DL'];
$tpl_vars['L_Add_multi_DL_Desc'] = $LANG['Add_multi_DL_Desc'];
$tpl_vars['L_PKG'] = $LANG['package'];
$tpl_vars['L_sel_type'] = $LANG['sel_or_type'];
$tpl_vars['err_message'] = $err_message;
$tpl_vars['content'] = $content;
?>

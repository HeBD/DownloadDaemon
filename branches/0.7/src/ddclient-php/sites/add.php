<?php
// Connect to Daemon
$socket = socket_create(AF_INET, SOCK_STREAM, 0);
$connect = connect_to_daemon($socket, $_COOKIE['ddclient_host'], $_COOKIE['ddclient_port'], $_COOKIE['ddclient_passwd'], $_COOKIE['ddclient_enc'], 5);

// Site Vars
$err_message = '';
$dl_list = '';

if($connect != 'SUCCESS') {
	$err_msg = msg_generate($LANG[$connect], 'error');
}else{
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
	} 
	if(isset($_POST['submit_multi'])) {
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


$tpl_vars['L_Title'] = $LANG['Title'];
$tpl_vars['L_URL'] = $LANG['URL'];
$tpl_vars['L_Add_single_DL'] = $LANG['Add_single_DL'];
$tpl_vars['L_Add_multi_DL'] = $LANG['Add_multi_DL'];
$tpl_vars['L_Add_multi_DL_Desc'] = $LANG['Add_multi_DL_Desc'];
$tpl_vars['err_message'] = $err_message;
?>
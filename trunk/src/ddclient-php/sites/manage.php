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
	if(isset($_GET['action'])){
		switch ($_GET['action']) {
    		case 'activate':
				send_all($socket, "DDP DL ACTIVATE " . $_GET['id']);
				$buf = "";
				recv_all($socket, $buf);
				if(mb_substr($buf, 0, 3) != "100") {
					$err_message .= msg_generate($LANG['ERR_DL_ACTIVATE'], 'error');
				}
        		break;
    		case 'deactivate':
				send_all($socket, "DDP DL DEACTIVATE " . $_GET['id']);
				$buf = "";
				recv_all($socket, $buf);
				if(mb_substr($buf, 0, 3) != "100") {
					$err_message .= msg_generate($LANG['ERR_DL_DEACTIVATE'], 'error');
				}
        		break;
    		case 'delete':
				send_all($socket, "DDP DL DEL " . $_GET['id']);
				$buf = "";
				recv_all($socket, $buf);
				if(mb_substr($buf, 0, 3) != "100") {
					$err_message .= msg_generate($LANG['ERR_DL_DEL'], 'error');
				}
        		break;
    		case 'del_file':
				send_all($socket, "DDP FILE DEL " . $_GET['id']);
				$buf = "";
				recv_all($socket, $buf);
				if(mb_substr($buf, 0, 3) != "100") {
					$err_message .= msg_generate($LANG['ERR_FILE_DEL'], 'error');
				}
    			break;
    		case 'move':
				send_all($socket, "DDP DL UP " . $_GET['id']);
				$buf = "";
				recv_all($socket, $buf);
				if(mb_substr($buf, 0, 3) != "100") {
					$err_message .= msg_generate($LANG['ERR_DL_UP'], 'error');
				}
    			break;
    		default:
    			break;
		}
	}
	$list = "";
	send_all($socket, "DDP DL LIST");
	recv_all($socket, $list);

	$download_index[] = array();
	$download_index = explode("\n", $list);

	$exp_dls[] = array();
	for($i = 0; $i < count($download_index); $i++) {
		$exp_dls[$i] = explode ( '|'  , $download_index[$i] );
	}

	for($i = 0; $i < count($exp_dls); $i++) {
		if($exp_dls[$i][0] == "") continue;
		if($exp_dls[$i][0] == "PACKAGE") {
			continue;
		}
		
		//Alternating <tr> Colors
		if($i%2 == 0) {
			$tr_class = 'even'; 
		}else{
			$tr_class = 'odd';
		}
	
		$dl_status = '';
		
		switch($exp_dls[$i][4]){
			case 'DOWNLOAD_RUNNING':
				if($exp_dls[$i][7] > 0 && $exp_dls[$i][8] == 'PLUGIN_SUCCESS') {
					$dl_status .= 'Download running. Waiting ' . $exp_dls[$i][7] . ' seconds.';
				} else if($exp_dls[$i][7] > 0 && $exp_dls[$i][8] != 'PLUGIN_SUCCESS') {
					$dl_status .= 'Error: ' . $exp_dls[$i][8] . ' Retrying in ' . $exp_dls[$i][7] . 's';
				} else {
					if($exp_dls[$i][6] == 0) {
						$dl_status .= 'Running, fetched ' . number_format($exp_dls[$i][5] / 1048576, 1) . 'MB';
					} else {
						$dl_status .= number_format($exp_dls[$i][9] / 1024) . 'kb/s - ' . number_format($exp_dls[$i][5] / 1048576, 1) . '/' . number_format($exp_dls[$i][6] / 1048576, 1) . 'MB - ' . number_format($exp_dls[$i][5] / $exp_dls[$i][6] * 100, 1) . '%';
					}
				}
				break;
			case 'DOWNLOAD_INACTIVE':
				if($exp_dls[$i][8] == 'PLUGIN_SUCCESS') {
					$dl_status .= 'Download Inactive';
				} else {
					$dl_status .= 'Inactive. Error: ' . $exp_dls[$i][8];
				}
				break;
			case 'DOWNLOAD_PENDING':
				if($exp_dls[$i][8] == 'PLUGIN_SUCCESS') {
					$dl_status .= 'Download Pending';
					$display_ddinactive_warn = true;
				} else {
					$dl_status .= 'Error: ' . $exp_dls[$i][8];
				}
				break;
			case 'DOWNLOAD_WAITING':
				$dl_status .= 'Have to wait ' . $exp_dls[$i][7] . ' seconds';
				break;
			case 'DOWNLOAD_FINISHED':
				$dl_status .= 'Download Finished';
				break;
			default:
				$dl_status .= 'Status not detected';	
		}
	
	if($exp_dls[$i][4] == "DOWNLOAD_INACTIVE") {
		$activate_button = '';
	} else {
		$activate_button = 'de';
	}

	send_all($socket, "DDP FILE GETPATH " . $exp_dls[$i][0]);
	$buf = "";
	recv_all($socket, $buf);
	if($buf != "") {
		$del_file = '<a href="index.php?site=manage&amp;action=del_file&amp;id={T_DL_ID}" title="{L_Delete_File}"><img src="{T_SITE_URL}/templates/default/css/images/delete_file.png" alt="{L_Delete_File}" /></a>';
	}else{
		$del_file = '';
	}
	
	$tpl_manage_vars = array(
		'T_Activate_Button' => $activate_button,
		'T_DelFile_Button' => $del_file,
		'T_TR_CLASS' => $tr_class,
		'T_DL_ID' => $exp_dls[$i][0],
		'T_DL_Date' => $exp_dls[$i][1],
		'T_DL_Title' => $exp_dls[$i][2],
		'T_DL_URL' => $exp_dls[$i][3],
		'T_DL_Class' => strtolower($exp_dls[$i][4]),
		'T_DL_Status' => $dl_status,
		'T_SITE_URL' => $tpl_vars['T_SITE_URL'],
		'L_Activate' => $LANG['Activate'],
		'L_Delete' => $LANG['Delete'],
		'L_Move' => $LANG['Move'],
		'L_Delete_File' => $LANG['Delete_File']
	);
	
	$dl_list .= template_parse('manage_line', $tpl_manage_vars);
}
}

$tpl_vars['L_Title'] = $LANG['Title'];
$tpl_vars['L_URL'] = $LANG['URL'];
$tpl_vars['L_ID'] = $LANG['ID'];
$tpl_vars['L_Date'] = $LANG['Date'];
$tpl_vars['L_Status'] = $LANG['Status'];
$tpl_vars['T_List'] = $dl_list;
$tpl_vars['err_message'] = $err_message;
?>

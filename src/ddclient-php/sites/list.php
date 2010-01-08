<?php
$err_message = '';
$dl_list = '';
$socket = socket_create(AF_INET, SOCK_STREAM, 0);
connect_to_daemon($socket, $_COOKIE['ddclient_host'], $_COOKIE['ddclient_port'], $_COOKIE['ddclient_passwd'], $_COOKIE['ddclient_enc'], 5);
$list = '';
send_all($socket, 'DDP DL LIST');
recv_all($socket, $list);

$download_count = substr_count($list, "\n");
$download_index = array();
$download_index = explode("\n", $list);
$exp_dls[] = array();
for($i = 0; $i < $download_count; $i++) {
	$exp_dls[$i] = explode( '|'  , $download_index[$i] );
}

$display_ddinactive_warn = false;

for($i = 0; $i < $download_count; $i++) {
	$dl_list .= '<tr><td>'.$exp_dls[$i][0].'</td><td>'.$exp_dls[$i][1].'</td><td>'.$exp_dls[$i][2].'</td><td>'.$exp_dls[$i][3].'</td>';
	if($exp_dls[$i][4] == 'DOWNLOAD_RUNNING') {
		if($exp_dls[$i][7] > 0 && $exp_dls[$i][8] == 'PLUGIN_SUCCESS') {
			$dl_list .= '<td bgcolor="lime">';
			$dl_list .= 'Download running. Waiting ' . $exp_dls[$i][7] . ' seconds.';
		} else if($exp_dls[$i][7] > 0 && $exp_dls[$i][8] != 'PLUGIN_SUCCESS') {
			$dl_list .= '<td bgcolor="red">';
			$dl_list .= 'Error: ' . $exp_dls[$i][8] . ' Retrying in ' . $exp_dls[$i][7] . 's';
		} else {
			$dl_list .= '<td bgcolor="lime">';
			if($exp_dls[$i][6] == 0) {
				$dl_list .= 'Running, fetched ' . number_format($exp_dls[$i][5] / 1048576, 1) . 'MB';
			} else {
				$dl_list .= 'Running@' . number_format($exp_dls[$i][9] / 1024) . 'kb/s - ' . number_format($exp_dls[$i][5] / $exp_dls[$i][6] * 100, 1) . '% - ' . number_format($exp_dls[$i][5] / 1048576, 1) . 'MB/' . number_format($exp_dls[$i][6] / 1048576, 1) . 'MB';
			}
		}
	} else if($exp_dls[$i][4] == 'DOWNLOAD_INACTIVE') {
		if($exp_dls[$i][8] == 'PLUGIN_SUCCESS') {
			$dl_list .= '<td bgcolor="yellow">';
			$dl_list .= 'Download Inactive';
		} else {
			$dl_list .= '<td bgcolor="red">';
			$dl_list .= 'Inactive. Error: ' . $exp_dls[$i][8];
		}		
	} else if($exp_dls[$i][4] == 'DOWNLOAD_PENDING') {
		if($exp_dls[$i][8] == 'PLUGIN_SUCCESS') {
			$dl_list .= '<td>';
			$dl_list .= 'Download Pending';
			$display_ddinactive_warn = true;
		} else {
			$dl_list .= '<td bgcolor="red">';
			$dl_list .= 'Error: ' . $exp_dls[$i][8];
		}
	} else if($exp_dls[$i][4] == 'DOWNLOAD_WAITING') {
		$dl_list .= '<td bgcolor="yellow">';
		$dl_list .= 'Have to wait ' . $exp_dls[$i][7] . ' seconds';
	} else if($exp_dls[$i][4] == 'DOWNLOAD_FINISHED') {
		$dl_list .= '<td bgcolor="green">';
		$dl_list .= 'Download Finished';
	} else {
		$dl_list .= '<td>Status not detected';
	} 
	$dl_list .= '</td>';	
	$dl_list .= '</tr>';
}

	if($display_ddinactive_warn) {
		$buf = '';
		send_all($socket, 'DDP VAR GET downloading_active');
		recv_all($socket, $buf);
		if($buf == 0 || $buf == 'false') {
			$dl_list .= '<br><div style="align:center; color:red">Downloading is currently inactive. Click <a href="conf_mgmt.php">here</a> to activate it.</div>';

		}
	}

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
	'DL_List' => $dl_list,
	'err_message' => $err_message,
);
?>

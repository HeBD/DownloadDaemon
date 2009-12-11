<!--
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
-->

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
       "http://www.w3.org/TR/html4/loose.dtd">
<?php
include("functional.php");
?>

<html>
	<head>
		<title>DownloadDaemon Manager - General Configuration</title>
	</head>
<body>
	
	<?php include("header.php"); ?>

	<div align="center">
	<br>
	<a href="conf_mgmt.php">General Configuration</a> | <a href="conf_reconnect.php">Reconnect Setup</a> | <a href="conf_premium.php">Premium account setup</a>
	<br>
<?php
	$socket = socket_create(AF_INET, SOCK_STREAM, 0);
	$ret = connect_to_daemon($socket);
	conn_error_quit($ret);

	echo "<br><br>";

	if(isset($_POST['change_pw'])) {
		send_all($socket, "DDP VAR SET mgmt_password=" . $_POST['old_pw'] . ";" . $_POST['new_pw']);
		$buf = "";
		recv_all($socket, $buf);
		if(mb_substr($buf, 0, 3) != "100") {
			echo "The Password could not be changed<br>";
			exit;
		}		
	}

	if(isset($_POST['downloading_active'])) {
		if($_POST['downloading_active'] == "Activate Downloading") {
			send_all($socket, "DDP VAR SET downloading_active=1");
		} else {
			send_all($socket, "DDP VAR SET downloading_active=0");
		}
		$buf = "";
		recv_all($socket, $buf);
		if(mb_substr($buf, 0, 3) != "100") {
			echo "Failed to set DownloadDaemons download activity status";
		}
	}

	if(isset($_POST['apply'])) {
		$buf = "";
		send_all($socket, "DDP VAR SET download_timing_start = " . $_POST['download_timing_start']);
		recv_all($socket, $buf);
		send_all($socket, "DDP VAR SET download_timing_end = " . $_POST['download_timing_end']);
		recv_all($socket, $buf);
		send_all($socket, "DDP VAR SET download_folder = " . $_POST['download_folder']);
		recv_all($socket, $buf);
		send_all($socket, "DDP VAR SET simultaneous_downloads = " . $_POST['simultaneous_downloads']);
		recv_all($socket, $buf);
		send_all($socket, "DDP VAR SET log_level = " . $_POST['log_level']);
		recv_all($socket, $buf);
		send_all($socket, "DDP VAR SET log_file = " . $_POST['log_file']);
		recv_all($socket, $buf);	

	}

	echo "Change Password: <br>";
	echo "<form action=\"conf_mgmt.php\" method=\"post\">";
	echo "Old Password: <input type=\"text\" name=\"old_pw\"> New Password: <input type=\"text\" name=\"new_pw\"> <input type=\"submit\" name=\"change_pw\" value=\"Change Password\">";
	echo "</form>";
	echo "<br><br>";
	echo "<form action=\"conf_mgmt.php\" method=\"post\">";

	$buf = "";
	send_all($socket, "DDP VAR GET downloading_active");
	recv_all($socket, $buf);
	if($buf == "0") {
		echo "<input type=\"submit\" name=\"downloading_active\" value=\"Activate Downloading\">";
	} else {
		echo "<input type=\"submit\" name=\"downloading_active\" value=\"Deactivate Downloading\">";
	}

	echo "<br><br><br>";
	$dl_start = "";
	$dl_end = "";
	send_all($socket, "DDP VAR GET download_timing_start");
	recv_all($socket, $dl_start);
	send_all($socket, "DDP VAR GET download_timing_end");
	recv_all($socket, $dl_end);
	echo "You can force DownloadDaemon to only download at specific times. Therefore you can specifie a start time and an end time in the format hours:minutes.<br>";
	echo "Leave this fields empty if you want to allow DownloadDaemon to download permanently.<br>";
	echo "Start Time: <input type=\"text\" size=\"5\" name=\"download_timing_start\" value=\"" . $dl_start . "\"> ";
	echo "End Time: <input type=\"text\" size=\"5\" name=\"download_timing_end\" value=\"" . $dl_end . "\">";
	echo "<br><br><br>";
	$dl_folder = "";
	send_all($socket, "DDP VAR GET download_folder");
	recv_all($socket, $dl_folder);
	echo "This option specifies where finished downloads should be safed on the server:<br>";
	echo "Download folder: <input type=\"text\" size=\"40\" name=\"download_folder\" value=\"" . $dl_folder . "\">";
	echo "<br><br><br>";
	$sim_dls = "";
	send_all($socket, "DDP VAR GET simultaneous_downloads");
	recv_all($socket, $sim_dls);
	echo "Here you can specify how many downloads may run at the same time:<br>";
	echo "Simultaneous downloads: <input type=\"text\" size=\"5\" name=\"simultaneous_downloads\" value=\"" . $sim_dls . "\">";
	echo "<br><br><br>";
	$log_lvl = "";	
	send_all($socket, "DDP VAR GET log_level");
	recv_all($socket, $log_lvl);
	echo "This option specifies DownloadDaemons logging activity.<br>";
	echo "<select name=\"log_level\"><option value=\"DEBUG\" ";
	if($log_lvl == "DEBUG") {
		echo "SELECTED ";
	}
	echo ">Debug</option>";
	echo "<option value=\"WARNING\" ";
	if($log_lvl == "WARNING") {
		echo "SELECTED ";
	}
	echo ">Warning</option>";
	echo "<option value=\"SEVERE\" ";
	if($log_lvl == "SEVERE") {
		echo "SELECTED ";
	}
	echo ">Severe</option>";
	echo "<option value=\"OFF\" ";
	if($log_lvl == "OFF") {
		echo "SELECTED ";
	}
	echo ">Off</option></select>";
	echo "<br><br><br>";
	$log_file = "";
	send_all($socket, "DDP VAR GET log_file");
	recv_all($socket, $log_file);
	echo "This option specifies the destination for log-output and can either be a filename, \"stderr\" or \"stdout\" for local console output<br>";
	echo "Log file: <input type=\"text\" name=\"log_file\" value=\"". $log_file ."\" size=\"40\">";
	echo "<br><br><br>";
	echo "<input type=\"submit\" name=\"apply\" value=\"Apply\">";

	echo "</form>";


?>

	</div>
</body>
</html>

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
		<title>DownloadDaemon Manager - Download List</title>
		<meta HTTP-EQUIV="REFRESH" content="5; url=list.php">
	</head>
<body>
	
	<?php include("header.php"); ?>

	<div align="center">
		
<?php
	$socket = socket_create(AF_INET, SOCK_STREAM, 0);
	$ret = connect_to_daemon($socket);
	if($ret == "COOKIE") {
		echo "Cookies missing. If you have cookies disabled on your machine, please enable them. Else, try to log on again by clicking <a href=\"index.php\">here</a>";
		exit;
	} else if ($ret == "CONNECT") {
		echo "Coult not connect to DownloadDaemon. Maybe it is not running?";
		exit;
	} else if ($ret == "RECV") {
		echo "Successfully connected to the server, but data could not be received. Connection problem";
		exit;
	} else if ($ret == "PASSWD") {
		echo "You entered a wrong password. Please go back to the login page.";
		exit;
	}
	echo "<br><br>";

	$list = "";
	send_all($socket, "DDP DL LIST");
	recv_all($socket, $list);

	$download_count = substr_count($list, "\n");
	$download_index[] = array();
	$download_index = explode("\n", $list);
	$exp_dls[] = array();
	for($i = 0; $i < $download_count; $i++) {
		$exp_dls[$i] = explode ( '|'  , $download_index[$i] );
	}

	echo "<table border=\"1\">\n";
	echo "<tr><td>ID</td><td>Date</td><td>Title</td><td>URL</td><td>Status</td></tr>";
	
	for($i = 0; $i < $download_count; $i++) {
		echo "<tr>";
		for($j = 0; $j < 5; $j++) {
			switch($j) {
			case 0: echo "<td>" . $exp_dls[$i][$j] . "</td>"; break;
			case 1: echo "<td>" . $exp_dls[$i][$j] . "</td>"; break;
			case 2: echo "<td>" . $exp_dls[$i][$j] . "</td>"; break;
			case 3: echo "<td>" . $exp_dls[$i][$j] . "</td>"; break;
			case 4: 
				if($exp_dls[$i][4] == "DOWNLOAD_RUNNING") {
					
					if($exp_dls[$i][7] > 0 && $exp_dls[$i][8] == "PLUGIN_SUCCESS") {
						echo "<td bgcolor=\"lime\">";
						echo "Download running. Waiting " . $exp_dls[$i][7] . " seconds.";
					} else if($exp_dls[$i][7] > 0 && $exp_dls[$i][8] != "PLUGIN_SUCCESS") {
						echo "<td bgcolor=\"red\">";
						echo "Error: " . $exp_dls[$i][8] . " Retrying in " . $exp_dls[$i][7] . "s";
					} else {
						echo "<td bgcolor=\"lime\">";
						if($exp_dls[$i][6] == 0) {
							echo "Running, fetched " . number_format($exp_dls[$i][5] / 1048576, 1) . "MB";
						} else {
							echo "Running@" . number_format($exp_dls[$i][9] / 1024) . "kb/s - " . number_format($exp_dls[$i][5] / $exp_dls[$i][6] * 100, 1) . "% - " 
							      . number_format($exp_dls[$i][5] / 1048576, 1) . "MB/" . number_format($exp_dls[$i][6] / 1048576, 1) . "MB";
						}
					}
				} else if($exp_dls[$i][4] == "DOWNLOAD_INACTIVE") {
					if($exp_dls[$i][8] == "PLUGIN_SUCCESS") {
						echo "<td bgcolor=\"yellow\">";
						echo "Download Inactive";
					} else {
						echo "<td bgcolor=\"red\">";
						echo "Inactive. Error: " . $exp_dls[$i][8];
					}
		
				} else if($exp_dls[$i][4] == "DOWNLOAD_PENDING") {
					if($exp_dls[$i][8] == "PLUGIN_SUCCESS") {
						echo "<td>";
						echo "Download Pending";
					} else {
						echo "<td bgcolor=\"red\">";
						echo "Error: " . $exp_dls[$i][8];
					}
				} else if($exp_dls[$i][4] == "DOWNLOAD_WAITING") {
					echo "<td bgcolor=\"yellow\">";
					echo "Have to wait " . $exp_dls[$i][7] . " seconds";
				} else if($exp_dls[$i][4] == "DOWNLOAD_FINISHED") {
					echo "<td bgcolor=\"green\">";
					echo "Download Finished";
				} else {
					echo "<td>Status not detected";
				} 
				
				echo "</td>";
				break;	


			}

		}

		echo "</tr>";
	}

	echo "</table>"
?>
	</div>
</body>
</html>

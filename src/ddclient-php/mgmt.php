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
		<title>DownloadDaemon Manager - Download Management</title>
		<meta HTTP-EQUIV="REFRESH" content="5; url=mgmt.php">
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

	if(isset($_POST['activate'])) {
		send_all($socket, "DDP DL ACTIVATE " . $_POST['id']);
		$buf = "";
		recv_all($socket, $buf);
		if(mb_substr($buf, 0, 3) != "100") {
			echo "Could not activate download " . $_POST['id'];
			exit;
		}
	}

	if(isset($_POST['deactivate'])) {
		send_all($socket, "DDP DL DEACTIVATE " . $_POST['id']);
		$buf = "";
		recv_all($socket, $buf);
		if(mb_substr($buf, 0, 3) != "100") {
			echo "Could not deactivate download " . $_POST['id'];
			exit;
		}
	}
	
	if(isset($_POST['delete'])) {
		send_all($socket, "DDP DL DEL " . $_POST['id']);
		$buf = "";
		recv_all($socket, $buf);
		if(mb_substr($buf, 0, 3) != "100") {
			echo "Could not delete download " . $_POST['id'];
			exit;
		}
	}

	if(isset($_POST['delete_file'])) {
		send_all($socket, "DDP FILE DEL " . $_POST['id']);
		$buf = "";
		recv_all($socket, $buf);
		if(mb_substr($buf, 0, 3) != "100") {
			echo "Could not delete File " . $_POST['id'];
			exit;
		}
	}

	if(isset($_POST['up'])) {
		send_all($socket, "DDP DL UP " . $_POST['id']);
		$buf = "";
		recv_all($socket, $buf);
		if(mb_substr($buf, 0, 3) != "100") {
			echo "Could not move download upwards";
			exit;
		}
	}

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
	echo "<tr><td>ID</td><td>Date</td><td>Title</td><td>URL</td><td>Action</td></tr>";
	
	for($i = 0; $i < $download_count; $i++) {
		echo "<tr>";
		for($j = 0; $j < 5; $j++) {
			switch($j) {
			case 0: echo "<td>" . $exp_dls[$i][$j] . "</td>"; break;
			case 1: echo "<td>" . $exp_dls[$i][$j] . "</td>"; break;
			case 2: echo "<td>" . $exp_dls[$i][$j] . "</td>"; break;
			case 3: echo "<td>" . $exp_dls[$i][$j] . "</td>"; break;
			case 4: echo "<td>";
				echo "<form action=\"mgmt.php\" method=\"post\">";
				if($exp_dls[$i][4] == "DOWNLOAD_INACTIVE") {
					echo "<input type=\"submit\" name=\"activate\" value=\"Activate\">";
				} else {
					echo "<input type=\"submit\" name=\"deactivate\" value=\"Deactivate\">";
				}
				
				echo "<input type=\"submit\" name=\"delete\" value=\"Delete\">";

				echo "<input type=\"submit\" name=\"up\" value=\"Up\">";

				send_all($socket, "DDP FILE GETPATH " . $exp_dls[$i][0]);
				$buf = "";
				recv_all($socket, $buf);
				if($buf != "") {
					echo "<input type=\"submit\" name=\"delete_file\" value=\"Delete File\">";
				}

				echo "<input type=\"hidden\" name=\"id\" value=\"" . $exp_dls[$i][0] . "\">";
					



				echo "</form>";
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

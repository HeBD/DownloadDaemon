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

<html>
	<head>
		<title>DownloadDaemon Manager - Add Downloads</title>
		<link rel="shortcut icon" href="ddico.ico" type="image/x-icon">
	</head>
<body>
	
	<?php include("header.php"); include("functional.php"); ?>

	<div align="center">
	<?php if(!isset($_POST['submit_single']) && !isset($_POST['submit_multiple'])) { ?>
	<br>
	Add singe Download (no title necessary):
	<form action="add.php" method="post">
		Title: <input type="text" name="title" size="100"> <br>
		URL: <input type="text" name="url" size="100"> <br>
		<input type="submit" value="Add" name="submit_single">
	</form>
	<br><br>
	Here you can add several downloads at once.<br> If you want to specify a comment for a download, enter it as
	http://something.aa/bb|A Fancy Title.<br> This, however, is not necessary.<br>
	Titles/URLs:<br>
	<form action="add.php" method="post">
		 <textarea name="titles_urls" cols="120" rows="20"></textarea><br>
		<input type="submit" value="Add" name="submit_multiple">
	</form>
	<?php
	} else {
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
		echo "<br>";
		if(isset($_POST['submit_single'])) {
			send_all($socket, "DDP DL ADD " . $_POST['url'] . " " . $_POST['title']);
			$recv = "";
			recv_all($socket, $recv);
			if($recv[0] == "-") {
				echo "The Download could not be added. The URL is probably invalid.";
				exit;
			} else {
				echo "Successfully added download.";
			}
		

		} else {
			$list = $_POST['titles_urls'];
			$download_count = substr_count($list, "http://");
			if($download_count == 0) {
				$download_count = substr_count($list, "ftp://");
			}
			if($download_count == 0) {
				echo "No valid URL entered.";
				exit;
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
				echo "All downloads addedd successfully.";
			}

		}
	}
	?>

	</div>
</body>
</html>
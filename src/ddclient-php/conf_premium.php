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
		<title>DownloadDaemon Manager - Reconnect Setup</title>
	</head>
<body>
	
	<?php include("header.php"); ?>

	<div align="center">
	<br>
	<a href="conf_mgmt.php">General Configuration</a> | <a href="conf_reconnect.php">Reconnect Setup</a> | <a href="conf_premium.php">Premium account setup</a>
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

	if(isset($_POST['apply'])) {
		$buf = "";
		send_all($socket, "DDP PREMIUM SET " . $_POST['host'] . " " . $_POST['user'] . ";" . $_POST['pass']);
		recv_all($socket, $buf);
		echo "Successfully set premium credentials for " . $_POST['host'];
	}

	$buf = "";
	echo "<form action=\"conf_premium.php\" method=\"post\">";
	send_all($socket, "DDP PREMIUM LIST");
	recv_all($socket, $buf);
	$host_list = explode("\n", $buf);
	array $user_list = "";

	echo "Host: ";
	echo "<select name=\"host\">";
	echo "<option value=\"\"></option>";
	for($i = 0; $i != count($host_list); $i++) {
		echo "<option value=\"" . $host_list[$i] . "\"";
		echo ">" . $host_list[$i] . "</option>";
	}
	echo "</select>";
	echo "<br><br>";
	echo "Username: <input type=\"text\" name=\"user\"><br>";
	echo "Password: <input type=\"text\" name=\"pass\"><br>";
	echo "<input type=\"submit\" name=\"apply\" value=\"Apply\"><br>";	
	echo "</form>";

?>

	</div>
</body>
</html>

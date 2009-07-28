<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
       "http://www.w3.org/TR/html4/loose.dtd">
<?php
include("functional.php");
?>

<html>
	<head>
		<title>DownloadDaemon Manager</title>
		<meta HTTP-EQUIV="REFRESH" content=\"5; url=list.php\">
	</head>
<body>
	<div align="center">
	<h1>DownloadDaemon Manager</h1>
		
<?php
	$socket = socket_create(AF_INET, SOCK_STREAM, 0);
	connect_to_daemon($socket);
	if($socket == "COOKIE") {
		die("Cookies missing. If you have cookies disabled on your machine, please enable them. Else, try to log on again by clicking <a href=\"index.php\">here</a>");
	} else if ($socket == "CONNECT") {
		die("Coult not connect to DownloadDaemon. Maybe it is not running?");
	} else if ($socket == "RECV") {
		die("Successfully connected to the server, but data could not be received. Connection problem");
	} else if( $socket == "PASSWD") {
		die("You entered a wrong password. Please go back to the login page.");
	}
	echo "<br><br>";

	$list = "";
	send_all($socket, "DDP GET LIST");
	recv_all($socket, $list);
	echo $list;
?>
	</div>
</body>

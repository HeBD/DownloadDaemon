<?php
	$past = time() - 100;

	setcookie("ddclient_host", "", $past);
	setcookie("ddclient_port", "", $past);
	setcookie("ddclient_passwd", "", $past);
	header("Location: index.php");
?>

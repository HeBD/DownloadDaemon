<?php
	if(isset($_POST['submit'])) {
		$day = 86400 + time();
	
		$host = $_POST['host'];
		$port = $_POST['port'];
		$passwd = $_POST['passwd'];
		$enc = "0";
		if($_POST['encrypt'] == "1") {
			$enc = 1;
		}
		

		if(!is_numeric($port)) {
			die("Invalid Port");
		}
	
		if( !preg_match('/^\d+\.\d+\.\d+\.\d+$/', $host) ) {
			$host = gethostbyname($host);
		}

		setcookie("ddclient_host", $host, $day);
		setcookie("ddclient_port", $port, $day);
		setcookie("ddclient_passwd", $passwd, $day);
		setcookie("ddclient_enc", $enc, $day);
		header("Location: list.php");
	}

?>
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
		<title>DownloadDaemon Login</title>
		<link rel="shortcut icon" href="ddico.ico" type="image/x-icon">
	</head>

	<body>
		<div align="center">
			<p>
				<h1>DownloadDaemon Login:</h1><br>
				<?php if(!isset($_POST['submit'])) { ?>
				<form action="index.php" method="post">
					<table border="0">
						<tr>
							<td>Host/Port:</td>
							<td><input type="text" name="host" value="127.0.0.1"><input type="text" name="port" value="56789" size="5"></td>
						</tr>
						<tr>
							<td>Password:</td>
							<td><input type="password" name="passwd"></td>
						</tr>
						<tr>
							<td>Force Encryption:</td>
							<td><input type="checkbox" name="encrypt" value="1"></td>
						</tr>		
					</table>
					<input type="submit" name="submit" value="Login">
				</form>	
				<?php } else { echo "Please stand by while we are connecting to the server. If you are not redirected automatically, please click <a href=\"list.php\">here</a>"; } ?>
			</p>
		</div>
	</body>
</html>
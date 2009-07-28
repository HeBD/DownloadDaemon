<?php
	if(isset($_POST['submit'])) {
		$day = 86400 + time();
	
		$host = $_POST['host'];
		$port = $_POST['port'];
		$passwd = $_POST['passwd'];

		if(!is_numeric($port)) {
			die("Invalid Port");
		}
	
		if( !preg_match('/^\d+\.\d+\.\d+\.\d+$/', $host) ) {
			$host = gethostbyname($host);
		}

		setcookie("ddclient_host", $host, $day);
		setcookie("ddclient_port", $port, $day);
		setcookie("ddclient_passwd", $passwd, $day);
		$redirect = true;
		//header("Location: list.php");
	}

?>

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
       "http://www.w3.org/TR/html4/loose.dtd">
<html>
	<head>
		<title>DownloadDaemon Login</title>
		<?php if($redirect) { echo "<meta HTTP-EQUIV=\"REFRESH\" content=\"0; url=list.php\">"; } ?>
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
						<td><input type="text" name="host" value="127.0.0.1"><input type="text" name="port" value="4321" size="5"></td>
					</tr>
					<tr>
						<td>Password:</td>
						<td><input type="password" name="passwd"></td>
					</tr>			
					</table>
					<input type="submit" name="submit" value="Login">
				</form>	
				<?php } else { echo "Please stand by while we are connecting to the server. If you are not redirected automatically, please click <a href=\"list.php\">here</a>"; } ?>
			</p>
		</div>
	</body>
</html>

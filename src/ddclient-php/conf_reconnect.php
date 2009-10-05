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
	<a href="conf_mgmt.php">General Configuration</a> | <a href="conf_reconnect.php">Reconnect Setup</a>
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
		send_all($socket, "DDP ROUTER SET reconnect_policy = " . $_POST['reconnect_policy']);
		recv_all($socket, $buf);
		send_all($socket, "DDP ROUTER SETMODEL " . $_POST['router_model']);
		recv_all($socket, $buf);
		send_all($socket, "DDP ROUTER SET router_ip = " . $_POST['router_ip']);
		recv_all($socket, $buf);
		send_all($socket, "DDP ROUTER SET router_username = " . $_POST['router_username']);
		recv_all($socket, $buf);
		if($_POST['router_password'] != "") {
			send_all($socket, "DDP ROUTER SET router_password = " . $_POST['router_password']);
			recv_all($socket, $buf);
		}	
	}
	if(isset($_POST['enable_reconnect'])) {
		$buf = "";
		send_all($socket, "DDP VAR SET enable_reconnect = 1");
		recv_all($socket, $buf);
	}
	if(isset($_POST['disable_reconnect'])) {
		$buf = "";
		send_all($socket, "DDP VAR SET enable_reconnect = 0");
		recv_all($socket, $buf);
	}

	$buf = "";
	echo "<form action=\"conf_reconnect.php\" method=\"post\">";
	send_all($socket, "DDP VAR GET enable_reconnect");
	recv_all($socket, $buf);
	if($buf == "1" || $buf == "true") {
		echo "<input type=\"submit\" name=\"disable_reconnect\" value=\"Disable Reconnecting\">";
	} else {
		echo "<input type=\"submit\" name=\"enable_reconnect\" value=\"Enable Reconnecting\">";
	}
	echo "<br><br>";
	send_all($socket, "DDP ROUTER GET reconnect_policy");
	$buf = "";
	recv_all($socket, $buf);
	echo "This option specifies DownloadDaemons reconnect activity: <br>";
	echo "HARD cancels all downloads if a reconnect is needed<br>";
	echo "CONTINUE only cancels downloads that can be continued after the reconnect<br>";
	echo "SOFT will wait until all other downloads are finished<br>";
	echo "PUSSY will only reconnect if there is no other choice (no other download can be started without a reconnect)<br>";
	echo "Reconnect Policy: <select name=\"reconnect_policy\"><option value=\"HARD\" ";
	if($buf == "HARD") {
		echo "SELECTED ";
	}
	echo ">HARD</option>";
	echo "<option value=\"CONTINUE\" ";
	if($buf == "CONTINUE") {
		echo "SELECTED ";
	}
	echo ">CONTINUE</option>";
	echo "<option value=\"SOFT\" ";
	if($buf == "SOFT") {
		echo "SELECTED ";
	}
	echo ">SOFT</option>";
	echo "<option value=\"PUSSY\" ";
	if($buf == "PUSSY") {
		echo "SELECTED ";
	}
	echo ">PUSSY</option></select>";
	echo "<br><br>";
	$buf = "";
	$model = "";
	send_all($socket, "DDP ROUTER GET router_model");
	recv_all($socket, $model);
	send_all($socket, "DDP ROUTER LIST");
	recv_all($socket, $buf);
	$model_list = explode("\n", $buf);
	
	echo "Router Model: ";
	echo "<select name=\"router_model\">";
	echo "<option value=\"\"></option>";
	for($i = 0; $i != count($model_list); $i++) {
		echo "<option value=\"" . $model_list[$i] . "\"";
		if($model_list[$i] == $model) {
			echo " SELECTED ";
		}
		echo ">" . $model_list[$i] . "</option>";
	}
	echo "</select>";
	echo "<br><br>";
	$buf = "";
	send_all($socket, "DDP ROUTER GET router_ip");
	recv_all($socket, $buf);
	echo "Router IP: <input type=\"text\" name=\"router_ip\" value=\"" . $buf . "\"><br>";
	send_all($socket, "DDP ROUTER GET router_username");
	recv_all($socket, $buf);
	echo "Username: <input type=\"text\" name=\"router_username\" value=\"" . $buf . "\"><br>";
	echo "Password: <input type=\"text\" name=\"router_password\"><br>";
	echo "<br><input type=\"submit\" name=\"apply\" value=\"Apply\"><br>";
	echo "</form>";

?>

	</div>
</body>
</html>

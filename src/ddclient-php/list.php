<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
       "http://www.w3.org/TR/html4/loose.dtd">
<?php
include("functional.php");
?>

<html>
	<head>
		<title>DownloadDaemon Manager</title>
		<meta HTTP-EQUIV="REFRESH" content="5; url=list.php">
	</head>
<body>
	<div align="center">
	<h1>DownloadDaemon Manager</h1>
		
<?php
	$socket = socket_create(AF_INET, SOCK_STREAM, 0);
	$ret = connect_to_daemon($socket);
	if($ret == "COOKIE") {
		die("Cookies missing. If you have cookies disabled on your machine, please enable them. Else, try to log on again by clicking <a href=\"index.php\">here</a>");
	} else if ($ret == "CONNECT") {
		die("Coult not connect to DownloadDaemon. Maybe it is not running?");
	} else if ($ret == "RECV") {
		die("Successfully connected to the server, but data could not be received. Connection problem");
	} else if ($ret == "PASSWD") {
		die("You entered a wrong password. Please go back to the login page.");
	}
	echo "<br><br>";

	$list = "";
	send_all($socket, "DDP GET LIST");
	recv_all($socket, $list);

	// returns the string without nn pattern ... substitute xx
	//echo str_replace("nn", "xx", $str);

	$download_count = substr_count($list, "\n");
	$download_index[] = array();
	$download_index = explode("\n", $list);
	$exp_dls[] = array();
	for($i = 0; $i < $download_count; $i++) {
		$exp_dls[$i] = explode ( '|'  , $download_index[$i] );
	}
	echo $exp_dls[0][2];

	echo "<table border=\"1\">\n";
	echo "<tr><td>ID</td><td>Date</td><td>Title</td><td>URL</td><td>Status</td>";
	
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
					echo "<td bgcolor=\"green\">";
					if($exp_dls[$i][7] > 0) {
						echo "Download running. Waiting " . $exp_dls[$i][7] . " seconds.";
					} else {
						echo "Download Running: " . number_format($exp_dls[$i][5] / $exp_dls[$i][6] * 100, 1) . "% - " 
						      . number_format($exp_dls[$i][5] / 1000000, 1) . "MB/" . number_format($exp_dls[$i][6] / 1000000, 1) . "MB";
					}
				} else if($exp_dls[$i][4] == "DOWNLOAD_INACTIVE") {
					if($exp_dls[$i][8] == "NO_ERROR") {
						echo "<td bgcolor=\"yellow\">";
						echo "Download Inactive";
					} else {
						echo "<td bgcolor=\"red\">";
						echo "Error: " . $exp_dls[$i][8];
					}
		
				} else if($exp_dls[$i][4] == "DOWNLOAD_PENDING") {
					if($exp_dls[$i][8] == "NO_ERROR") {
						echo "<td>";
						echo "Download Pending";
					} else {
						echo "<td bgcolor=\"red\">";
						echo "Error: " . $exp_dls[$i][8];
					}
				} else if($exp_dls[$i][4] == "DOWNLOAD_WAITING") {
					echo "<td bgcolor=\"yellow\">";
					echo "Have to wait " . $exp_dls[$i][7] . " seconds";
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


	// now we need to parse $list. schema:
	//1|Thu Jul 23 20:08:04 2009||http://rapidshare.com/files/144286726/worldshift_Ka0oS.part02.rar|DOWNLOAD_RUNNING|4086368|200000000|130|NO_ERROR 
	//2|Thu Jul 23 20:08:08 2009|aswekdfa|http://rapidshare.com/files/144286726/worldshift_Ka0oS.part02.rar|DOWNLOAD_INACTIVE|845088|200000000|0|NO_ERROR 
	//echo $list;
?>
	</div>
</body>

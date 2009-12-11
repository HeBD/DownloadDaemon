<!--
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
-->

<?php

function recv_all(&$socket, &$recv_buf) {
	
	$recv_buf = "";
	$recv_number = "";
	while(true) {
		if(!@socket_recv($socket, $recv_buf, 1,  0)) {
			return false;
		}
		if($recv_buf == ":") break;
		$recv_number = $recv_number . $recv_buf;
	}
	$recv_buf = "";
	$received_bytes = 0;
	$temp_recv = "";

	// receive all
	while(($received_bytes += socket_recv($socket, $temp_recv, (int)$recv_number - $received_bytes, 0)) < $recv_number) {
		$recv_buf .= $temp_recv;
		if($received_bytes < 0) {
			return false;
		}
	}
	// append the rest
	$recv_buf .= $temp_recv;
	return true;
	
}

function send_all(&$socket, $snd) {
	$to_send = strlen($snd) . ":" . $snd;
	if(@socket_send($socket, $to_send, strlen($to_send), 0)) {
		return true;
	}
	return false;
	
}


function connect_to_daemon(&$socket) {
	set_time_limit(0);
	if(!isset($_COOKIE['ddclient_host']) || !isset($_COOKIE['ddclient_port'])) {
		return "COOKIE";
	}
	
	$host = $_COOKIE['ddclient_host'];
	$port = $_COOKIE['ddclient_port'];
	$enc = $_COOKIE['ddclient_enc'];
	$pw = "";
	if(isset($_COOKIE['ddclient_passwd'])) {
		$pw = $_COOKIE['ddclient_passwd'];
	}

	if(!@socket_connect($socket, $host, $port)) {
		return "CONNECT";
	}
	
	@recv_all($socket, $recv_buf);

	if(mb_substr($recv_buf, 0, 3) == "100") {
		return "SUCCESS";
	}

	@send_all($socket, "ENCRYPT");
	$rnd = "";
	@recv_all($socket, $rnd);
	$result = "";
	if(mb_substr($rnd, 0, 3) != "102") {
		$rnd .= $pw;
		$enc_passwd = md5($rnd, true);
		send_all($socket, $enc_passwd);
		recv_all($socket, $result);
	} else {
		// use plain-text if allowed
		if($enc == "0") {
			socket_close($socket);
			$socket = socket_create(AF_INET, SOCK_STREAM, 0);
			if(!@socket_connect($socket, $host, $port)) {
				return "CONNECT";
			}
			recv_all($socket, $recv_buf);
			send_all($socket, $pw);
			recv_all($socket, $result);
		}
	}
	
	if(mb_substr($result, 0, 3) != "100") {
		return "PASSWD";
	} else {
		return "SUCCESS";
	}
}

function conn_error_quit($ret) {
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
		echo "You entered a wrong password or used an encryption method the server does not support. Please go back to the login page.";
		exit;
	}

}

?>


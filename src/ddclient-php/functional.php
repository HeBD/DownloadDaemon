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
	
	if(!@socket_connect($socket, $host, $port)) {
		return "CONNECT";
	}
	
	if(!@recv_all($socket, $recv_buf)) {
		return "RECV";
	}

	if(mb_substr($recv_buf, 0, 3) != "100") {
		$passwd = $_COOKIE['ddclient_passwd'];
		send_all($socket, $passwd);
		recv_all($socket, $recv_buf);
		if(mb_substr($recv_buf, 0, 3) != "100") {
			return "PASSWD";
		}
	}

	return "SUCCESS";
}

?>


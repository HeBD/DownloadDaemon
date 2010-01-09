<!--
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
-->
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
	<head>
		<title>{L_DD} - {L_Site}</title>
		<link rel="shortcut icon" href="templates/ddico.ico" type="image/x-icon">
		<link rel="stylesheet" href="templates/default/css/style.css" type="text/css" media="screen">
	</head>
	<body>
		<div id="wrapper">
			<div id="header">
				<h1>{L_DD} Manager</h1>
				<ul id="nav">
					<li><a href="index.php?site=add">{L_Add_DL}</a></li>
					<li><a href="index.php?site=list">{L_List_DL}</a></li>
					<li><a href="index.php?site=manage">{L_Manage_DL}</a></li>
					<li><a href="index.php?site=config">{L_Config_DD}</a></li>
					<li><a href="index.php?site=logout">{L_Logout}</a></li>
				</ul>
			</div>
			<div id="content">
				{err_message}

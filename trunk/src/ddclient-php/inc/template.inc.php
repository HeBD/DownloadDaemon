<?php

function get_template($file){
	if(file_exists('templates/'.TEMPLATE.'/'.$file.'.tpl')){
		$temp = file_get_contents('templates/'.TEMPLATE.'/header.tpl');
		$temp .= file_get_contents('templates/'.TEMPLATE.'/'.$file.'.tpl');
		$temp .= file_get_contents('templates/'.TEMPLATE.'/footer.tpl');
		return $temp;
	}else{
		return false;
	}
}

function parse_template($file, $array){
	$temp = get_template($file);
	if($temp){
		foreach ($array as $key => $value){
			$temp = str_replace('{'.$key.'}', $value, $temp);
		}
	}else{
		echo '\"templates/'.TEMPLATE.'/'.$file.'.tpl\" Does Not Exist.';
	}
	return $temp;
}
?>

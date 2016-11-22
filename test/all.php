<?php


function getvarTest($got=true, $expected=false) {
	if ($got === $expected) return;
	$trace = debug_backtrace()[0];
	echo "ERROR: FAILED!!\n\n";
	echo "FILE: $trace[file]\n";
	echo "LINE: $trace[line]\n\n";
	echo "EXPECTED:\n'";
	echo print_r($expected, true) . "'\n\n";
	echo "QUERY:\n'";
	echo print_r($got, true) . "'\n\n";
	exit(1);
}


function getvarError($exception, $expected) {
	if ($exception->getMessage() === $expected) return;
	$trace = debug_backtrace()[0];
	echo "ERROR: FAILED!!\n\n";
	echo "FILE: $trace[file]\n";
	echo "LINE: $trace[line]\n\n";
	echo "EXPECTED:\n";
	echo "'" . $expected . "'\n\n";
	echo "ERROR:\n";
	echo "'" . $exception->getMessage() . "'\n\n";
	exit(1);
}



require('get.php');
require('post.php');
require('string.php');
require('int.php');
require('float.php');
require('currency.php');

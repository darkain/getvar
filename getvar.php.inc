<?php

define('_GETVAR_BASIC',		0 <<  0);
define('_GETVAR_NOGET',		1 <<  0);
define('_GETVAR_NOPOST',	1 <<  1);
define('_GETVAR_SQLSAFE',	1 <<  2);
define('_GETVAR_HTMLSAFE',	1 <<  3);
define('_GETVAR_URLSAFE',	1 <<  4);
define('_GETVAR_NOTRIM',	1 <<  5);
define('_GETVAR_BREAK',		1 <<  6);
define('_GETVAR_MD5BIN',	1 << 29);
define('_GETVAR_MD5',		1 << 30);
define('_GETVAR_HTML_SQL',	_GETVAR_SQLSAFE | _GETVAR_HTMLSAFE);


class getvar {

	public function __invoke($name, $flags=false) {
		//attempt to get the value from POST
		if (($flags & _GETVAR_NOPOST) == 0) {
			if (isset($_POST[$name])) $value = $_POST[$name];
		}

		//attempt to get the value from GET
		if (!isset($value)  &&  (($flags & _GETVAR_NOGET) == 0)) {
			if (isset($_GET[$name])) $value = $_GET[$name];
		}

		//value not found
		if (!isset($value)) return NULL;

		//clean and return value
		return $this->_clean($value, $flags);
	}


	public function server($name, $default=NULL, $flags=false) {
		if (!isset($_SERVER[$name])) return $default;
		return $this->_clean($_SERVER[$name], $flags);
	}


	public function session($name, $default=NULL, $flags=false) {
		if (!isset($_SESSION[$name])) return $default;
		return $this->_clean($_SESSION[$name], $flags);
	}


	public function item($name, $flags=false) {
		return $this($name, $flags);
	}


	public function lists($name, $separator=',', $flags=false) {
		$value = explode($separator, $this($name, $flags));
		foreach ($value as $key => &$item) {
			$item = trim($item);
			if ($item === '') unset($value[$key]);
		}
		return $value;
	}


	public function int($name, $flags=false) {
		$value = $this($name, $flags);
		if (!strcasecmp($value, 'true')) return 1;
		return (int) $value;
	}


	public function intArray($name, $flags=false) {
		$value = $this($name, $flags);
		if (!is_array($value)) $value = array();
		foreach ($value as &$item) $item = (int) $item;
		return $value;
	}


	public function intList($name, $separator=',', $flags=false) {
		$value = $this->lists($name, $separator, $flags);
		foreach ($value as &$item) $item = (int) $item;
		return $value;
	}


	public function float($name, $flags=false) {
		$value = $this($name, $flags);
		if (!strcasecmp($value, 'true')) return 1.0;
		return (float) $value;
	}


	public function floatArray($name, $flags=false) {
		$value = $this($name, $flags);
		if (!is_array($value)) $value = array();
		foreach ($value as &$item) $item = (float) $item;
		return $value;
	}


	public function floatList($name, $separator=',', $flags=false) {
		$value = $this->lists($name, $separator, $flags);
		foreach ($value as &$item) $item = (float) $item;
		return $value;
	}


	public function string($name, $flags=false) {
		return $this->_utf8((string)$this($name, $flags));
	}


	public function stringUpper($name, $flags=false) {
		return strtoupper($this->string($name, $flags));
	}


	public function stringLower($name, $flags=false) {
		return strtolower($this->string($name, $flags));
	}


	public function stringNull($name, $flags=false) {
		$text = $this->string($name, $flags);
		if (empty($text)  &&  $text !== '0') return NULL;
		return $text;
	}


	public function stringArray($name, $flags=false) {
		$value = $this($name, $flags);
		if (!is_array($value)) $value = array();
		foreach ($value as &$item) $item = $this->_utf8((string)$item);
		return $value;
	}


	public function stringList($name, $separator=',', $flags=false) {
		$value = $this->lists($name, $separator, $flags);
		foreach ($value as &$item) $item = $this->_utf8((string)$item);
		return $value;
	}


	public function id($name='id') {
		return (int) $this($name, _GETVAR_BASIC);
	}


	public function bool($name) {
		$value = getvar::string($name, _GETVAR_BASIC);
		if (!strcasecmp($value, 'true'))	return true;
		if (!strcasecmp($value, 'false'))	return false;
		return (bool) $value;
	}


	public function hash($name='hash', $binary=false) {
		$hash = $this($name, _GETVAR_BASIC);
		if (!strlen($hash)) return false;
		if (!ctype_xdigit($hash)) return false;
		return $binary ? hex2bin($hash) : $hash;
	}


	public function hashArray($name='hash', $binary=false) {
		$value = $this($name, _GETVAR_BASIC);
		if (!is_array($value)) $value = array();
		foreach ($value as $key => &$hash) {
			if (!ctype_xdigit($hash)) {
				unset($value[$key]);
			} else if ($binary) {
				$hash = hex2bin($hash);
			}
		}
		return $value;
	}


	public function hashList($name, $separator=',', $flags=false) {
		$value = $this->lists($name, $separator, $flags);
		foreach ($value as $key => $hash) if (!ctype_xdigit($hash)) unset($value[$key]);
		return $value;
	}


	public function binary($name='hash') {
		return $this->hash($name, true);
	}


	public function binaryArray($name='hash') {
		return $this->hashArray($name, true);
	}


	public function timestamp($name) {
		$value = $this($name, _GETVAR_BASIC);
		if (ctype_digit($value)) return (int) $value;
		return strtotime($value);
	}



	protected function _utf8($value) {
		if (mb_detect_encoding($value, 'UTF-8', true)) return $value;
		return utf8_encode($value);
	}



	protected function _clean(&$value, $flags) {
		if ($flags === false) $flags = $this->default;

		if (is_array($value)) {
			foreach ($value as &$item) {
				$this->_clean($item, $flags);
			} unset($item);
			return $value;
		}

		//strip slashes if magic quotes are enabled
		if (get_magic_quotes_gpc()) {
			$value = stripslashes($value);
		}

		//convert non breaking space character
		if (($flags & _GETVARB_REAK) == 0) {
			$value = preg_replace('/[\pZ\pC]/u', ' ', $value);
		}

		//trim the value
		if (($flags & _GETVAR_NOTRIM) == 0) {
			$value = trim($value);
		}

		//if no value, return
		if (is_null($value)) return $value;

		//convert to MD5 checksum (binary)
		if (($flags & _GETVAR_MD5BIN) > 0) {
			$value = md5($value, true);
		}

		//convert to MD5 checksum
		if (($flags & _GETVAR_MD5) > 0) {
			$value = md5($value);
		}

		//clean out HTML special characters
		if (($flags & _GETVAR_HTMLSAFE) > 0) {
			$value = htmlspecialchars($value, ENT_QUOTES);
		}

		//clean out URL paramater special characters
		if (($flags & _GETVAR_URLSAFE) > 0) {
			$value = rawurlencode($value);
		}

		//prevent SQL injection
		if (($flags & _GETVAR_SQLSAFE) > 0) {
			if ($this->pudl) {
				$value = $this->pudl->escape($value);
			} else {
				$value = @mysql_real_escape_string($value);
			}
		}

		return $value;
	}


	public $pudl	= false;
	public $default	= _GETVAR_BASIC;
}

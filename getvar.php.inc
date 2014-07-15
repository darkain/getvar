<?php

define('_GETVAR_BASIC',		0 <<  0);
define('_GETVAR_NOGET',		1 <<  0);
define('_GETVAR_NOPOST',	1 <<  1);
define('_GETVAR_SQLSAFE',	1 <<  2);
define('_GETVAR_HTMLSAFE',	1 <<  3);
define('_GETVAR_URLSAFE',	1 <<  4);
define('_GETVAR_NOTRIM',	1 <<  5);
define('_GETVAR_MD5BIN',	1 << 29);
define('_GETVAR_MD5',		1 << 30);
define('_GETVAR_HTML_SQL',	_GETVAR_SQLSAFE | _GETVAR_HTMLSAFE);


class getvar {

	public function server($name, $default=NULL, $flags=false) {
		if (!isset($_SERVER[$name])) return $default;
		return $this->_clean($_SERVER[$name], $flags);
	}


	public function session($name, $default=NULL, $flags=false) {
		if (!isset($_SESSION[$name])) return $default;
		return $this->_clean($_SESSION[$name], $flags);
	}


	public function item($name, $flags=false) {
		$value = null;

		//attempt to get the value from POST
		if (($flags & _GETVAR_NOPOST) == 0) {
			if (isset($_POST[$name])) $value = $_POST[$name];
		}

		//attempt to get the value from GET
		if (is_null($value)  &&  (($flags & _GETVAR_NOGET) == 0)) {
			if (isset($_GET[$name])) $value = $_GET[$name];
		}

		//clean and return value
		return $this->_clean($value, $flags);
	}


	public function int($name, $flags=false) {
		$value = getvar::item($name, $flags);
		if (strcasecmp($value, 'true') === 0) return 1;
		return (int) $value;
	}


	public function intArray($name, $flags=false) {
		$value = getvar::item($name, $flags);
		if (!is_array($value)) $value = array();
		foreach ($value as $key => &$item) {
			$item = (int) $item;
		}
		return $value;
	}


	public function float($name, $flags=false) {
		$value = getvar::item($name, $flags);
		if (strcasecmp($value, 'true') === 0) return 1.0;
		return (float) $value;
	}


	public function floatArray($name, $flags=false) {
		$value = getvar::item($name, $flags);
		if (!is_array($value)) $value = array();
		foreach ($value as $key => &$item) {
			$item = (float) $item;
		}
		return $value;
	}


	public function string($name, $flags=false) {
		return (string) getvar::item($name, $flags);
	}


	public function stringArray($name, $flags=false) {
		$value = getvar::item($name, $flags);
		if (!is_array($value)) $value = array();
		foreach ($value as $key => &$item) {
			$item = (string) $item;
		}
		return $value;
	}


	public function id($name='id') {
		return (int) getvar::item($name, _GETVAR_BASIC);
	}


	public function bool($name) {
		$value = getvar::string($name, _GETVAR_BASIC);
		if (strtolower($value) === 'true')  $value = true;
		if (strtolower($value) === 'false') $value = false;
		return (bool) $value;
	}


	public function hash($name='hash') {
		$hash = getvar::item($name, _GETVAR_BASIC);
		if (!strlen($hash)) return false;
		if (!ctype_xdigit($hash)) return false;
		return $hash;
	}


	public function hashArray($name) {
		$value = getvar::item($name, _GETVAR_BASIC);
		if (!is_array($value)) $value = array();
		foreach ($value as $key => &$hash) {
			if (!ctype_xdigit($hash)) $hash = '';
		}
		return $value;
	}


	public function timestamp($name) {
		$value = getvar::item($name, _GETVAR_BASIC);
		if (ctype_digit($value)) return (int) $value;
		return strtotime($value);
	}
	

	protected function _clean(&$value, $flags) {
		if ($flags === false) $flags = $this->default;

		if (is_array($value)) {
			foreach ($value as $key => &$item) {
				$this->_clean($item, $flags);
			}
			return $value;
		}

		//strip slashes if magic quotes are enabled
		if (get_magic_quotes_gpc()) {
			$value = stripslashes($value);
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
				$value = $this->pudl->safe($value);
			} else {
				$value = mysql_real_escape_string($value);
			}
		}

		return $value;
	}


	public $pudl = false;
	public $default = _GETVAR_BASIC;
}

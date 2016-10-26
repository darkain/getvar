<?php

define('_GETVAR_BASIC',		0 <<  0);
define('_GETVAR_NOGET',		1 <<  0);
define('_GETVAR_NOPOST',	1 <<  1);
define('_GETVAR_SQLSAFE',	1 <<  2);
define('_GETVAR_HTMLSAFE',	1 <<  3);
define('_GETVAR_URLSAFE',	1 <<  4);
define('_GETVAR_NOTRIM',	1 <<  5);
define('_GETVAR_BREAK',		1 <<  6);
define('_GETVAR_NULL',		1 <<  7);
define('_GETVAR_CURRENCY',	1 <<  8);
define('_GETVAR_MD5BIN',	1 << 29);
define('_GETVAR_MD5',		1 << 30);
define('_GETVAR_HTML_SQL',	_GETVAR_SQLSAFE | _GETVAR_HTMLSAFE);


class getvar implements ArrayAccess {

	public function __construct($default=_GETVAR_BASIC) {
		$this->default = $default;
	}


	public function __invoke($name, $flags=false) {
		//ATTEMPT TO GET THE VALUE FROM POST
		if (!($flags & _GETVAR_NOPOST)) {
			if (isset($_POST[$name])) $value = $_POST[$name];
		}

		//ATTEMPT TO GET THE VALUE FROM GET
		if (!isset($value)  &&  !($flags & _GETVAR_NOGET)) {
			if (isset($_GET[$name])) $value = $_GET[$name];
		}

		//VALUE NOT FOUND
		if (!isset($value)) {
			return ($flags & _GETVAR_NULL) ? NULL : '';
		}

		//CLEAN AND RETURN VALUE
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


	public function sessionClear($name, $default=NULL, $flags=false) {
		$return = $this->session($name, $default, $flags);
		unset($_SESSION[$name]);
		return $return;
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
		$value = (float) $value;
		if (is_nan($value) || is_infinite($value)) return 0.0;
		return $value;
	}


	public function floatArray($name, $flags=false) {
		$value = $this($name, $flags);
		if (!is_array($value)) $value = array();
		foreach ($value as &$item) {
			$item = (float) $item;
			if (is_nan($item) || is_infinite($item)) $item = 0.0;
		}
		return $value;
	}


	public function floatList($name, $separator=',', $flags=false) {
		$value = $this->lists($name, $separator, $flags);
		foreach ($value as &$item) {
			$item = (float) $item;
			if (is_nan($item) || is_infinite($item)) $item = 0.0;
		}
		return $value;
	}


	public function currency($name, $flags=false) {
		if ($flags === false) $flags = 0;
		return $this->float($name, $flags | _GETVAR_CURRENCY);
	}


	public function currencyArray($name, $flags=false) {
		if ($flags === false) $flags = 0;
		return $this->floatArray($name, $flags | _GETVAR_CURRENCY);
	}


	public function currencyList($name, $flags=false) {
		if ($flags === false) $flags = 0;
		return $this->floatList($name, $flags | _GETVAR_CURRENCY);
	}


	public function string($name, $flags=false) {
		return $this->_utf8((string)$this($name, $flags));
	}


	public function upper($name, $flags=false) {
		return strtoupper($this->string($name, $flags));
	}


	public function stringUpper($name, $flags=false) {
		return strtoupper($this->string($name, $flags));
	}


	public function lower($name, $flags=false) {
		return strtolower($this->string($name, $flags));
	}


	public function stringLower($name, $flags=false) {
		return strtolower($this->string($name, $flags));
	}


	public function stringNull($name, $flags=false) {
		if ($flags === false) $flags = $this->default;
		$value = $this($name, $flags|_GETVAR_NULL);
		if ($value === NULL) return NULL;
		return ($value === '') ? NULL : $this->_utf8((string)$value);
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


	public function id($name='id', $flags=false) {
		return (int) $this($name, $flags);
	}


	public function password($name='password', $flags=false) {
		$password = $this($name, $flags);
		unset($_GET[$name], $_POST[$name], $_REQUEST[$name]);
		return $password;
	}


	public function bool($name, $flags=false) {
		$value = $this($name, $flags);
		if (!strcasecmp($value, 'true'))	return true;
		if (!strcasecmp($value, 'false'))	return false;
		return (bool) $value;
	}


	public function hash($name='hash', $binary=false, $flags=false) {
		$hash = $this($name, $flags);
		if (!strlen($hash)) return false;
		if (!ctype_xdigit($hash)) return false;
		return $binary ? hex2bin($hash) : $hash;
	}


	public function hashArray($name='hash', $binary=false, $flags=false) {
		$value = $this($name, $flags);
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


	public function binary($name='hash', $flags=false) {
		return $this->hash($name, true, $flags);
	}


	public function binaryArray($name='hash', $flags=false) {
		return $this->hashArray($name, true, $flags);
	}


	public function timestamp($name, $flags=false) {
		$value = $this($name, $flags);
		if (ctype_digit($value)) return (int) $value;
		return strtotime($value);
	}


	protected function _utf8($value) {
		if (mb_detect_encoding($value, 'UTF-8', true)) return $value;
		return utf8_encode($value);
	}



	protected function _clean($value, $flags=false) {
		if ($flags === false) $flags = $this->default;

		if (is_array($value)) {
			foreach ($value as &$item) {
				$item = $this->_clean($item, $flags);
			} unset($item);
			return $value;
		}

		//IF NO VALUE, RETURN
		if (is_null($value)) return $value;

		//STRIP SLASHES IF MAGIC QUOTES ARE ENABLED
		if (get_magic_quotes_gpc()) {
			$value = stripslashes($value);
		}

		//CONVERT NON BREAKING SPACE CHARACTER
		if (($flags & _GETVAR_BREAK) == 0) {
			$value = str_replace("\xC2\xA0", ' ', $value);
		}

		//TRIM THE VALUE
		if (($flags & _GETVAR_NOTRIM) == 0) {
			$value = trim($value);
		}

		//REMOVE CURRENCY SYMBOLS
		if (($flags & _GETVAR_CURRENCY) > 0) {
			$value = preg_replace('/^[\$\s\x{A2}-\x{A5}\x{20A0}-\x{20CF}\x{10192}]+/u', '', $value);
		}

		//CONVERT TO MD5 CHECKSUM (BINARY)
		if (($flags & _GETVAR_MD5BIN) > 0) {
			$value = md5($value, true);
		}

		//CONVERT TO MD5 CHECKSUM
		if (($flags & _GETVAR_MD5) > 0) {
			$value = md5($value);
		}

		//CLEAN OUT HTML SPECIAL CHARACTERS
		if (($flags & _GETVAR_HTMLSAFE) > 0) {
			$value = htmlspecialchars($value, ENT_QUOTES);
		}

		//CLEAN OUT URL PARAMATER SPECIAL CHARACTERS
		if (($flags & _GETVAR_URLSAFE) > 0) {
			$value = rawurlencode($value);
		}

		//PREVENT SQL INJECTION
		if (($flags & _GETVAR_SQLSAFE) > 0) {
			if (!empty($this->pudl)) {
				$value = $this->pudl->escape($value);
			} else {
				$value = @mysql_real_escape_string($value);
			}
		}

		return $value;
	}


	public function __set($key, $value) {
		throw new Exception('Cannot set values on class getvar');
	}


	public function offsetSet($key, $value) {
		throw new Exception('Cannot set values on class getvar');
	}


	public function &__get($key) {
		return $this($key);
	}


	public function offsetGet($key) {
		return $this($key);
	}


	public function __isset($key) {
		if (!($this->default & _GETVAR_NOPOST)) {
			if (isset($_POST[$key])) return true;
		}
		if (!($this->default & _GETVAR_NOGET)) {
			if (isset($_GET[$key])) return true;
		}
		return false;
	}


	public function offsetExists($key) {
		return isset($this->{$key});
	}


	public function __unset($key) {
		if (!($this->default & _GETVAR_NOPOST)) {
			unset($_POST[$key]);
		}
		if (!($this->default & _GETVAR_NOGET)) {
			unset($_GET[$key]);
		}
	}


	public function offsetUnset($key) {
		unset($this->{$key});
	}



	public			$default;
	public			$pudl		= NULL;
	public static	$version	= 'Getvar 2.5.0';
}

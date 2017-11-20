<?php


define('_GETVAR_BASIC',		0 <<  0);
define('_GETVAR_NOGET',		1 <<  0);
define('_GETVAR_NOPOST',	1 <<  1);
define('_GETVAR_HTMLSAFE',	1 <<  3);
define('_GETVAR_URLSAFE',	1 <<  4);
define('_GETVAR_NOTRIM',	1 <<  5);
define('_GETVAR_NODOUBLE',	1 <<  6);
define('_GETVAR_BREAK',		1 <<  7);
define('_GETVAR_NULL',		1 <<  8);
define('_GETVAR_CURRENCY',	1 <<  9);




class getvar implements ArrayAccess {


	public function __construct($default=_GETVAR_BASIC) {
		$this->default = $default;
	}




	public function __invoke($name=false, $flags=false, $recurse=false) {
		if ($flags === false) $flags = $this->default;

		//RECURSIVE SEARCH
		if (is_array($name)  ||  is_object($name)) {
			foreach ($name as $item) {
				$value = $this($item, $flags, true);
				if (!is_null($value)) break;
			}
			$name = 0;
		}

		//ATTEMPT TO GET THE VALUE FROM POST
		if (!isset($value)  &&  !($flags & _GETVAR_NOPOST)) {
			if (is_bool($name)) return $this->post($name);

			if ($this->type() === 'application/json') {
				$names = explode('/', $name);
				$value = $this->post(true);
				foreach ($names as $item) {
					$value = isset($value[$item]) ? $value[$item] : NULL;
				}

			} else if (isset($_POST[$name])) {
				$value = $_POST[$name];
			}
		}

		//ATTEMPT TO GET THE VALUE FROM GET
		if (!isset($value)  &&  !($flags & _GETVAR_NOGET)) {
			if ($name === false) return $this->get();
			if (isset($_GET[$name])) $value = $_GET[$name];
		}

		//HANDLE RECURSIVE SEARCHING
		if (!isset($value)  &&  $recurse) return NULL;

		//VALUE NOT FOUND
		if (!isset($value)  ||  is_null($value)  ||  $value === '') {
			return ($flags & _GETVAR_NULL) ? NULL : '';
		}

		//CLEAN AND RETURN VALUE
		return $this->_clean($value, $flags);
	}




	public function get() {
		if ($this->rawget === NULL) {
			$this->rawget = $this->server('QUERY_STRING', false);
		}
		return $this->rawget;
	}




	public function post($object=false) {
		if ($this->rawpost === NULL) {
			$this->rawpost = @file_get_contents('php://input');
		}
		if (!$object) return $this->rawpost;

		if ($this->rawjson === NULL) {
			$this->rawjson = @json_decode(
				$this->rawpost,
				true,
				512,
				JSON_BIGINT_AS_STRING
			);
		}
		return $this->rawjson;
	}




	public function type() {
		if ($this->type === NULL) {
			$this->type = $this->server('CONTENT_TYPE');
		}
		return $this->type;
	}




	public function flags($flags=false) {
		$return = $this->default;
		if ($flags !== false) $this->default = $flags;
		return $return;
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
		if ($value === NULL) return NULL;
		if (!strcasecmp($value, 'true')) return 1;
		return (int) $value;
	}




	public function intNull($name, $flags=false) {
		if ($flags === false) $flags = $this->default;
		return $this->int($name, $flags|_GETVAR_NULL);
	}




	public function intArray($name, $flags=false) {
		$value = $this($name, $flags);
		if (!is_array($value)) $value = array();
		foreach ($value as &$item) {
			if ($item === NULL) continue;
			if (!strcasecmp($item, 'true')) $item = 1;
			$item = (int) $item;
		}
		return $value;
	}




	public function intList($name, $separator=',', $flags=false) {
		$value = $this->lists($name, $separator, $flags);
		foreach ($value as &$item) {
			if ($item === NULL) continue;
			if (!strcasecmp($item, 'true')) $item = 1;
			$item = (int) $item;
		}
		return $value;
	}




	public function float($name, $flags=false) {
		$value = $this($name, $flags);
		if ($value === NULL) return NULL;
		if (!strcasecmp($value, 'true')) return 1.0;
		$value = (float) $value;
		if (is_nan($value) || is_infinite($value)) return 0.0;
		return $value;
	}




	public function floatNull($name, $flags=false) {
		if ($flags === false) $flags = $this->default;
		return $this->float($name, $flags|_GETVAR_NULL);
	}




	public function floatArray($name, $flags=false) {
		$value = $this($name, $flags);
		if (!is_array($value)) $value = array();
		foreach ($value as &$item) {
			if ($item === NULL) continue;
			if (!strcasecmp($item, 'true')) $item = 1.0;
			$item = (float) $item;
			if (is_nan($item) || is_infinite($item)) $item = 0.0;
		}
		return $value;
	}




	public function floatList($name, $separator=',', $flags=false) {
		$value = $this->lists($name, $separator, $flags);
		foreach ($value as &$item) {
			if ($item === NULL) continue;
			if (!strcasecmp($item, 'true')) $item = 1.0;
			$item = (float) $item;
			if (is_nan($item) || is_infinite($item)) $item = 0.0;
		}
		return $value;
	}




	public function currency($name, $flags=false) {
		if ($flags === false) $flags = 0;
		return $this->float($name, $flags | _GETVAR_CURRENCY);
	}




	public function currencyNull($name, $flags=false) {
		if ($flags === false) $flags = 0;
		return $this->float($name, $flags | _GETVAR_CURRENCY | _GETVAR_NULL);
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
		foreach ($value as &$item) {
			if ($item === NULL) continue;
			$item = $this->_utf8((string)$item);
		}
		return $value;
	}




	public function stringList($name, $separator=',', $flags=false) {
		$value = $this->lists($name, $separator, $flags);
		foreach ($value as &$item) {
			if ($item === NULL) continue;
			$item = $this->_utf8((string)$item);
		}
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
		if ($value === NULL) return NULL;
		if (!strcasecmp($value, 'true'))	return true;
		if (!strcasecmp($value, 'false'))	return false;
		if (!strcasecmp($value, 'null'))	return false;
		if (!strcasecmp($value, 'nil'))		return false;
		return (bool) $value;
	}




	public function hash($name='hash', $binary=false, $flags=false) {
		$hash = $this($name, $flags);
		if ($hash === NULL)			return NULL;
		if (!strlen($hash))			return false;
		if (!ctype_xdigit($hash))	return false;
		return $binary ? hex2bin($hash) : $hash;
	}




	public function hashArray($name='hash', $binary=false, $flags=false) {
		$value = $this($name, $flags);
		if (!is_array($value)) $value = array();
		foreach ($value as $key => &$hash) {
			if ($hash === NULL) continue;
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
		foreach ($value as $key => $hash) {
			if ($hash === NULL) continue;
			if (!ctype_xdigit($hash)) unset($value[$key]);
		}
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
		if ($value === NULL) return NULL;
		if (ctype_digit($value)) return (int) $value;
		return strtotime($value);
	}




	protected function _utf8($value) {
		if ($value === NULL) return NULL;

		return extension_loaded('mbstring')
				? mb_convert_encoding($value, 'UTF-8', 'UTF-8')
				: iconv('UTF-8', 'UTF-8//TRANSLIT', $value);
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
		if ($value === NULL) return $value;

		//CONVERT NON BREAKING SPACE CHARACTER
		if (($flags & _GETVAR_BREAK) == 0) {
			$value = str_replace("\xC2\xA0", ' ', $value);
		}

		//TRIM THE VALUE
		if (($flags & _GETVAR_NOTRIM) == 0) {
			$value = trim($value);
		}

		//REMOVE DOUBLE SPACES
		if (($flags & _GETVAR_NODOUBLE) > 0) {
			$value = preg_replace('/  +/', ' ', $value);
		}

		//REMOVE CURRENCY SYMBOLS
		if (($flags & _GETVAR_CURRENCY) > 0) {
			$value = preg_replace('/^[\$\s\x{A2}-\x{A5}\x{20A0}-\x{20CF}\x{10192}]+/u', '', $value);
		}

		//CLEAN OUT HTML SPECIAL CHARACTERS
		if (($flags & _GETVAR_HTMLSAFE) > 0) {
			$value = htmlspecialchars($value, ENT_QUOTES);
		}

		//CLEAN OUT URL PARAMATER SPECIAL CHARACTERS
		if (($flags & _GETVAR_URLSAFE) > 0) {
			$value = rawurlencode($value);
		}

		return $value;
	}




	public function __set($key, $value) {
		throw new Exception('Cannot set values on class getvar');
	}




	public function offsetSet($key, $value) {
		throw new Exception('Cannot set values on class getvar');
	}




	public function __get($key) {
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
	private			$rawget		= NULL;
	private			$rawpost	= NULL;
	private			$rawjson	= NULL;
	private			$type		= NULL;
	public static	$version	= 'Getvar 2.8.3';

}

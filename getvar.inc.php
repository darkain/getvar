<?php



////////////////////////////////////////////////////////////////////////////////
// ALL SUPPORTED GETVAR FLAGS
////////////////////////////////////////////////////////////////////////////////
define('_GETVAR_BASIC',		0 <<  0);
define('_GETVAR_NOGET',		1 <<  0);
define('_GETVAR_NOPOST',	1 <<  1);
define('_GETVAR_NOTRIM',	1 <<  5);
define('_GETVAR_NODOUBLE',	1 <<  6);
define('_GETVAR_UNICODE',	1 <<  7);
define('_GETVAR_NULL',		1 <<  8);
define('_GETVAR_CURRENCY',	1 <<  9);




////////////////////////////////////////////////////////////////////////////////
// HANDLE PHP VERSION SPECIFIC IMPLEMENTATIONS
////////////////////////////////////////////////////////////////////////////////
if (version_compare(PHP_VERSION, '8.0.0') >= 0) {
	require_once('modern.php');
} else {
	require_once('legacy.php');
}




////////////////////////////////////////////////////////////////////////////////
// THE GETVAR CLASS ITSELF
////////////////////////////////////////////////////////////////////////////////
class getvar implements ArrayAccess {
	use getvar_trait;




	////////////////////////////////////////////////////////////////////////////
	// CONSTRUCTOR - OPTIONALLY SET THE DEFAULT FLAGS
	////////////////////////////////////////////////////////////////////////////
	public function __construct($default=_GETVAR_BASIC) {
		$this->_default = $default;
	}




	////////////////////////////////////////////////////////////////////////////
	// PHP MAGIC METHOD - INVOKE
	// IF NO PARAMETERS SPECIFIED, GET EITHER THE ENTIRE $_GET OR $_POST DATA
	// IF NAME IS SPECIFIED, GET THAT PARTICULAR KEY FROM $_GET OR $_POST DATA
	// $FLAGS - OPTIONAL - HOW TO PROCESS THE DATA BEFORE RETURNING
	// $RECURSE - OPTIONAL - SEARCH RECURSIVELY FOR VALUE
	////////////////////////////////////////////////////////////////////////////
	public function __invoke($name=false, $flags=false, $recurse=false) {
		if (is_callable($flags)) {
			$callback	= $flags;
			$flags		= NULL;
		}

		if ($flags === false  ||  $flags === NULL) {
			$flags		= $this->_default;
		}

		// RECURSIVE SEARCH
		if (is_array($name)  ||  is_object($name)) {
			foreach ($name as $item) {
				$value = $this($item, $flags, true);
				if (!is_null($value)) break;
			}
			$name = 0;
		}

		// ATTEMPT TO GET THE VALUE FROM POST
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

			} else if (isset($this->_rawjson[$name])) {
				$value = $this->_rawjson[$name];
			}
		}

		// ATTEMPT TO GET THE VALUE FROM GET
		if (!isset($value)  &&  !($flags & _GETVAR_NOGET)) {
			if ($name === false) return $this->get();
			if (isset($_GET[$name])) $value = $_GET[$name];
		}

		// HANDLE RECURSIVE SEARCHING
		if (!isset($value)  &&  $recurse) return NULL;

		// HANDLE CUSTOM CALLBACK
		if (!empty($callback)) {
			return $callback(isset($value) ? $value : NULL);
		}

		// VALUE NOT FOUND
		if (!isset($value)  ||  is_null($value)  ||  $value === '') {
			return ($flags & _GETVAR_NULL) ? NULL : '';
		}

		// CLEAN AND RETURN VALUE
		return $this->clean($value, $flags);
	}




	////////////////////////////////////////////////////////////////////////////
	// RETURN THE RAW QUERY STRING SENT BY CLIENT
	////////////////////////////////////////////////////////////////////////////
	public function get() {
		if ($this->_rawget === NULL) {
			$this->_rawget = $this->server('QUERY_STRING', false);
		}
		return $this->_rawget;
	}




	////////////////////////////////////////////////////////////////////////////
	// RETURN THE RAW POST FORM DATA SENT BY CLIENT
	////////////////////////////////////////////////////////////////////////////
	public function post($object=false) {
		if ($this->_rawpost === NULL) {
			$this->_rawpost = @file_get_contents('php://input');
		}

		if ($object === false) return $this->_rawpost;

		if ($this->_rawjson === NULL) {
			$this->_rawjson = @json_decode(
				($object === true ? $this->_rawpost : $this($object)),
				true,
				512,
				JSON_BIGINT_AS_STRING
			);
		}
		return $this->_rawjson;
	}




	////////////////////////////////////////////////////////////////////////////
	// RETURN THE CONTENT_TYPE HEADER SEND BY CLIENT
	////////////////////////////////////////////////////////////////////////////
	public function type() {
		if ($this->_type === NULL) {
			$this->_type = $this->server('CONTENT_TYPE');
		}
		return $this->_type;
	}




	////////////////////////////////////////////////////////////////////////////
	// RETURNS THE CURRENT DEFAULT FLAGS
	// $FLAGS - OPTIONAL - SETS NEW DEFAULT FLAGS
	////////////////////////////////////////////////////////////////////////////
	public function flags($flags=false) {
		$return = $this->_default;
		if ($flags !== false) $this->_default = $flags;
		return $return;
	}




	////////////////////////////////////////////////////////////////////////////
	// RETURNS A VALUE BY $NAME FROM THE $_SERVER SUPERGLOBAL
	////////////////////////////////////////////////////////////////////////////
	public function server($name, $default=NULL, $flags=false) {
		if (!array_key_exists($name, $_SERVER)) return $default;
		return $this->clean($_SERVER[$name], $flags);
	}




	////////////////////////////////////////////////////////////////////////////
	// RETURNS A VALUE BY $NAME FROM THE $_SESSION SUPERGLOBAL
	////////////////////////////////////////////////////////////////////////////
	public function session($name, $default=NULL, $flags=false) {
		if (!array_key_exists($name, $_SESSION)) return $default;
		return $this->clean($_SESSION[$name], $flags);
	}




	////////////////////////////////////////////////////////////////////////////
	// RETURNS A VALUE BY $NAME FROM THE $_SESSION SUPERGLOBAL
	// ADDITIONALLY, THE VALUE IS REMOVED FROM THE $_SESSION SUPERGLOBAL
	////////////////////////////////////////////////////////////////////////////
	public function sessionClear($name, $default=NULL, $flags=false) {
		$return = $this->session($name, $default, $flags);
		unset($_SESSION[$name]);
		return $return;
	}




	////////////////////////////////////////////////////////////////////////////
	// LEGACY METHOD - ALIAS OF INVOKE
	////////////////////////////////////////////////////////////////////////////
	public function item($name, $flags=false) {
		return $this($name, $flags);
	}




	////////////////////////////////////////////////////////////////////////////
	// RETURNS AN ARRAY FROM A VALUE BY $NAME, EXPLODED ON $SEPARATOR
	////////////////////////////////////////////////////////////////////////////
	public function lists($name, $separator=',', $flags=false) {
		$value = explode($separator, $this($name, $flags));
		foreach ($value as $key => &$item) {
			$item = trim($item);
			if ($item === '') unset($value[$key]);
		}
		return $value;
	}




	////////////////////////////////////////////////////////////////////////////
	// USE PHP'S BUILT IN FILTER_VAR TO SANITIZE INPUT
	// https://www.php.net/manual/en/function.filter-var.php
	////////////////////////////////////////////////////////////////////////////
	public function filter($name, $filter, $options=0, $flags=false) {
		return filter_var($this($name, $flags), $filter, $options);
	}




	////////////////////////////////////////////////////////////////////////////
	// USE PHP'S BUILT IN FILTER_VAR TO SANITIZE AN ARRAY OF INPUTS
	// https://www.php.net/manual/en/function.filter-var.php
	////////////////////////////////////////////////////////////////////////////
	public function filterArray($name, $filter, $options=0, $flags=false) {
		$value = $this($name, $flags);
		if (!is_array($value)) $value = [];
		foreach ($value as &$item) {
			if ($item === NULL) continue;
			$item = filter_var($item, $filter, $options);
		}
		return $value;
	}




	////////////////////////////////////////////////////////////////////////////
	// RETURN AN INTEGER VALUE BY $NAME
	////////////////////////////////////////////////////////////////////////////
	public function int($name, $flags=false) {
		$value = $this($name, $flags);
		if ($value === NULL) return NULL;

		//	BOOLEAN [TRUE] AS STRING
		if (!strcasecmp($value, 'true'))	return 1;

		//	SOME FORM ITEMS PASS 'ON' WHEN VALUE TRUE
		if (!strcasecmp($value, 'on'))		return 1;

		//	SOME FRAMEWORKS PASS 'OK' WHEN VALUE IS TRUE
		if (!strcasecmp($value, 'ok'))		return 1;

		return (int) $value;
	}




	////////////////////////////////////////////////////////////////////////////
	// SAME AS INT(), BUT RETURNS NULL INSTEAD OF 0 ON MISSING/BAD VALUES
	////////////////////////////////////////////////////////////////////////////
	public function intNull($name, $flags=false) {
		if ($flags === false) $flags = $this->_default;
		return $this->int($name, $flags|_GETVAR_NULL);
	}




	////////////////////////////////////////////////////////////////////////////
	// RETURN AN ARRAY OF INTEGERS PASSED WITH THE SAME NAME FROM CLIENT
	////////////////////////////////////////////////////////////////////////////
	public function intArray($name, $flags=false) {
		$value = $this($name, $flags);
		if (!is_array($value)) $value = [];
		foreach ($value as &$item) {
			if ($item === NULL) continue;
			if (!strcasecmp($item, 'true')) $item = 1;
			$item = (int) $item;
		}
		return $value;
	}




	////////////////////////////////////////////////////////////////////////////
	// RETURNS AN INTEGER ARRAY FROM A VALUE BY $NAME, EXPLODED ON $SEPARATOR
	////////////////////////////////////////////////////////////////////////////
	public function intList($name, $separator=',', $flags=false) {
		$value = $this->lists($name, $separator, $flags);
		foreach ($value as &$item) {
			if ($item === NULL) continue;
			if (!strcasecmp($item, 'true')) $item = 1;
			$item = (int) $item;
		}
		return $value;
	}




	////////////////////////////////////////////////////////////////////////////
	// GET AN INTEGER ID, 0 IF NOT FOUND OR IMPROPER TYPE
	////////////////////////////////////////////////////////////////////////////
	public function id($name='id', $flags=false) {
		return (int) $this($name, $flags);
	}




	////////////////////////////////////////////////////////////////////////////
	// RETURN AN INTEGER VALUE BY $NAME
	////////////////////////////////////////////////////////////////////////////
	public function float($name, $flags=false) {
		$value = $this($name, $flags);
		if ($value === NULL) return NULL;

		//	BOOLEAN [TRUE] AS STRING
		if (!strcasecmp($value, 'true'))	return 1.0;

		//	SOME FORM ITEMS PASS 'ON' WHEN VALUE TRUE
		if (!strcasecmp($value, 'on'))		return 1.0;

		//	SOME FRAMEWORKS PASS 'OK' WHEN VALUE IS TRUE
		if (!strcasecmp($value, 'ok'))		return 1.0;

		//	CONVERT VALUE FROM STRING TO FLOAT
		$value = (float) $value;

		//	FIX FOR A BUG IN EARLY VERSIONS OF PHP 7.0
		if (is_nan($value) || is_infinite($value)) return 0.0;

		return $value;
	}




	////////////////////////////////////////////////////////////////////////////
	// SAME AS FLOAT(), BUT RETURNS NULL INSTEAD OF 0 ON MISSING/BAD VALUES
	////////////////////////////////////////////////////////////////////////////
	public function floatNull($name, $flags=false) {
		if ($flags === false) $flags = $this->_default;
		return $this->float($name, $flags|_GETVAR_NULL);
	}




	////////////////////////////////////////////////////////////////////////////
	// RETURN AN ARRAY OF FLOATS PASSED WITH THE SAME NAME FROM CLIENT
	////////////////////////////////////////////////////////////////////////////
	public function floatArray($name, $flags=false) {
		$value = $this($name, $flags);
		if (!is_array($value)) $value = [];
		foreach ($value as &$item) {
			if ($item === NULL) continue;
			if (!strcasecmp($item, 'true')) $item = 1.0;
			$item = (float) $item;
			if (is_nan($item) || is_infinite($item)) $item = 0.0;
		}
		return $value;
	}




	////////////////////////////////////////////////////////////////////////////
	// RETURNS A FLOAT ARRAY FROM A VALUE BY $NAME, EXPLODED ON $SEPARATOR
	////////////////////////////////////////////////////////////////////////////
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




	////////////////////////////////////////////////////////////////////////////
	// SAME AS FLOAT(), BUT REMOVES SPACES AND CURRENCY SYMBOLS
	////////////////////////////////////////////////////////////////////////////
	public function currency($name, $flags=false) {
		if ($flags === false) $flags = 0;
		return $this->float($name, $flags | _GETVAR_CURRENCY);
	}




	////////////////////////////////////////////////////////////////////////////
	// SAME AS CURRENCY(), BUT RETURNS NULL ON MISSING/BAD VALUES
	////////////////////////////////////////////////////////////////////////////
	public function currencyNull($name, $flags=false) {
		if ($flags === false) $flags = 0;
		return $this->float($name, $flags | _GETVAR_CURRENCY | _GETVAR_NULL);
	}




	////////////////////////////////////////////////////////////////////////////
	// RETURN AN ARRAY OF CURRENCY VALUES PASSED WITH THE SAME NAME FROM CLIENT
	////////////////////////////////////////////////////////////////////////////
	public function currencyArray($name, $flags=false) {
		if ($flags === false) $flags = 0;
		return $this->floatArray($name, $flags | _GETVAR_CURRENCY);
	}




	////////////////////////////////////////////////////////////////////////////
	// RETURNS A CURRENCY ARRAY FROM A VALUE BY $NAME, EXPLODED ON $SEPARATOR
	////////////////////////////////////////////////////////////////////////////
	public function currencyList($name, $flags=false) {
		if ($flags === false) $flags = 0;
		return $this->floatList($name, $flags | _GETVAR_CURRENCY);
	}




	////////////////////////////////////////////////////////////////////////////
	// SAME AS INVOKE(), BUT FORCES CONVERSION TO STRING
	////////////////////////////////////////////////////////////////////////////
	public function string($name, $flags=false) {
		return (string)$this($name, $flags);
	}




	////////////////////////////////////////////////////////////////////////////
	// SAME AS STRING(), BUT FORCES VALUE TO ALL UPPER-CASE CHARACTERS
	////////////////////////////////////////////////////////////////////////////
	public function upper($name, $flags=false) {
		return strtoupper($this->string($name, $flags));
	}




	////////////////////////////////////////////////////////////////////////////
	// LEGACY METHOD - ALIAS OF UPPER()
	////////////////////////////////////////////////////////////////////////////
	public function stringUpper($name, $flags=false) {
		return strtoupper($this->string($name, $flags));
	}




	////////////////////////////////////////////////////////////////////////////
	// SAME AS STRING(), BUT FORCES VALUE TO ALL LOWER-CASE CHARACTERS
	////////////////////////////////////////////////////////////////////////////
	public function lower($name, $flags=false) {
		return strtolower($this->string($name, $flags));
	}




	////////////////////////////////////////////////////////////////////////////
	// LEGACY METHOD - ALIAS OF LOWER()
	////////////////////////////////////////////////////////////////////////////
	public function stringLower($name, $flags=false) {
		return strtolower($this->string($name, $flags));
	}




	////////////////////////////////////////////////////////////////////////////
	// SAME AS STRING(), BUT RETURNS NULL ON MISSING/BAD VALUES
	////////////////////////////////////////////////////////////////////////////
	public function stringNull($name, $flags=false) {
		if ($flags === false) $flags = $this->_default;
		$value = $this($name, $flags|_GETVAR_NULL);
		if ($value === NULL) return NULL;
		return ($value === '') ? NULL : ((string)$value);
	}




	////////////////////////////////////////////////////////////////////////////
	// RETURN AN ARRAY OF STRINGS PASSED WITH THE SAME NAME FROM CLIENT
	////////////////////////////////////////////////////////////////////////////
	public function stringArray($name, $flags=false) {
		$value = $this($name, $flags);
		if (!is_array($value)) $value = [];
		foreach ($value as &$item) {
			if ($item === NULL) continue;
			$item = (string)$item;
		}
		return $value;
	}




	////////////////////////////////////////////////////////////////////////////
	// RETURNS A STRING ARRAY FROM A VALUE BY $NAME, EXPLODED ON $SEPARATOR
	////////////////////////////////////////////////////////////////////////////
	public function stringList($name, $separator=',', $flags=false) {
		$value = $this->lists($name, $separator, $flags);
		foreach ($value as &$item) {
			if ($item === NULL) continue;
			$item = (string)$item;
		}
		return $value;
	}




	////////////////////////////////////////////////////////////////////////////
	// PROCESS AND RETURN SEARCH TERMS
	////////////////////////////////////////////////////////////////////////////
	public function search($name='search', $flags=false) {
		$term	= $this->string($name, $flags);
		$term	= str_replace('+', ' ', $term);
		$term	= preg_replace('/\s\s+/', ' ', $term);
		return trim($term);
	}




	////////////////////////////////////////////////////////////////////////////
	// GIVEN A STRINGARRAY $KEY AND STRINGARRAY $VALUE,
	// COMBINE THEM INTO AN ARRAY
	////////////////////////////////////////////////////////////////////////////
	public function combine($key, $value, $flags=false) {
		$keys	= $this->stringArray($key,		$flags);
		$values	= $this->stringArray($value,	$flags);
		$return	= [];
		foreach ($keys as $id => $item) {
			if ($item === ''  ||  $item === NULL) continue;
			$return[$item] = isset($values[$id]) ? $values[$id] : '';
		}
		return $return;
	}




	////////////////////////////////////////////////////////////////////////////
	// GET A JSON VALUE BY $NAME, AND CONVERT IT TO AN ARRAY
	////////////////////////////////////////////////////////////////////////////
	public function json($name, $flags=false) {
		$json = $this($name, $flags);
		return @json_decode($json, true, 512, JSON_BIGINT_AS_STRING);
	}




	////////////////////////////////////////////////////////////////////////////
	// GET AN ITEM BY $NAME, AND THEN UNSET IT SO IT CANNOT BE READ AGAIN
	////////////////////////////////////////////////////////////////////////////
	public function password($name='password', $flags=false) {
		$password = $this($name, $flags);
		unset($this->{$name});
		return $password;
	}




	////////////////////////////////////////////////////////////////////////////
	// GET A BOOLEAN VALUE, CONVERTING COMMON STRING NAMES
	////////////////////////////////////////////////////////////////////////////
	public function bool($name, $flags=false) {
		$value = $this($name, $flags);
		if ($value === NULL) return NULL;

		//	'FALSE' (NOTE: 'TRUE' IS HANDLED BY DEFAULT CLAUSE)
		if (!strcasecmp($value, 'false'))		return false;

		//	'NULL'
		if (!strcasecmp($value, 'null'))		return false;

		//	'NIL' - LANGUAGES SUCH AS [GO], [RUBY], AND [LUA]
		if (!strcasecmp($value, 'nil'))			return false;

		//	'NONE' - LANGUAGES SUCH AS [PYTHON]
		if (!strcasecmp($value, 'none'))		return false;

		//	'NAN' - NOT A NUMBER
		if (!strcasecmp($value, 'nan'))			return false;

		//	'UNDEFINED' - LANGUAGES SUCH AS [JAVASCRIPT]
		if (!strcasecmp($value, 'undefined'))	return false;

		//	DEFAULT - USE PHP'S BUILT IN CONVERSION
		return !empty($value);
	}




	////////////////////////////////////////////////////////////////////////////
	// GET A HASH VALUE, OR NULL ON ERROR
	////////////////////////////////////////////////////////////////////////////
	public function hash($name='hash', $binary=false, $flags=false) {
		$hash = $this($name, $flags);
		if ($hash === NULL)			return NULL;
		if (!strlen($hash))			return false;
		if (!ctype_xdigit($hash))	return false;
		return $binary ? hex2bin($hash) : $hash;
	}




	////////////////////////////////////////////////////////////////////////////
	// GET AN ARRAY OF HASHES
	////////////////////////////////////////////////////////////////////////////
	public function hashArray($name='hash', $binary=false, $flags=false) {
		$value = $this($name, $flags);
		if (!is_array($value)) $value = [];
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




	////////////////////////////////////////////////////////////////////////////
	// GET AN ARRAY OF HASHES FROM A CHARACTER SEPARATED LIST
	////////////////////////////////////////////////////////////////////////////
	public function hashList($name, $separator=',', $flags=false) {
		$value = $this->lists($name, $separator, $flags);
		foreach ($value as $key => $hash) {
			if ($hash === NULL) continue;
			if (!ctype_xdigit($hash)) unset($value[$key]);
		}
		return $value;
	}




	////////////////////////////////////////////////////////////////////////////
	// GET A VALUE CONVERTED FROM HEX INTO BINARY
	////////////////////////////////////////////////////////////////////////////
	public function binary($name='hash', $flags=false) {
		return $this->hash($name, true, $flags);
	}




	////////////////////////////////////////////////////////////////////////////
	// GET AN ARRAY OF VALUES CONVERTED FROM HEX TO BINARY
	////////////////////////////////////////////////////////////////////////////
	public function binaryArray($name='hash', $flags=false) {
		return $this->hashArray($name, true, $flags);
	}




	////////////////////////////////////////////////////////////////////////////
	// RETURN A UNIX TIMESTAMP
	// PERFORM AUTOMATIC CONVERSION FROM DATE/TIME STRINGS
	////////////////////////////////////////////////////////////////////////////
	public function timestamp($name, $flags=false) {
		$value = $this($name, $flags);
		if ($value === NULL) return NULL;
		if (ctype_digit($value)) return (int) $value;
		return strtotime($value);
	}




	////////////////////////////////////////////////////////////////////////////
	// CLEAN A VALUE
	////////////////////////////////////////////////////////////////////////////
	public function clean($value, $flags=false) {
		if ($flags === false) $flags = $this->_default;

		if (is_array($value)) {
			foreach ($value as &$item) {
				$item = $this->clean($item, $flags);
			} unset($item);
			return $value;
		}

		//IF NO VALUE, RETURN
		if ($value === NULL) return $value;


		//VALIDATE UTF-8 (MULTI-BYTE STRING EXTENSION)
		if (extension_loaded('mbstring')) {
			if (mb_detect_encoding($value, 'UTF-8', true) !== 'UTF-8') {
				$value = mb_convert_encoding($value, 'ISO-8859-1', 'UTF-8');
			}

		//VALIDATE UTF-8 (ICONV EXTENSION)
		} else if (extension_loaded('iconv')) {
			$value = iconv('UTF-8', 'UTF-8//TRANSLIT', $value);

		//VALIDATE UTF-8 MANUALLY
		} else {
			$value = preg_replace(
				'/((?:
					[\x00-\x7F] |
					[\xC2-\xDF][\x80-\xBF] |
					[\xE0-\xEF][\x80-\xBF]{2} |
					[\xF0-\xF4][\x80-\xBF]{3}
				)+)|./x',
				'$1',
				$value
			);
		}


		//CONVERT UNICODE SPACE CHARACTERS TO NORMAL [SPACE]
		if (($flags & _GETVAR_UNICODE) == 0) {
			$value = preg_replace(
				'/[\x{A0}\x{2000}-\x{200D}\x{202F}\x{205F}\x{2060}\x{3000}\x{FEFF}]+/u',
				' ', $value
			);
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
			$value = preg_replace(
				'/[,\$\s\x{A2}-\x{A5}\x{20A0}-\x{20CF}\x{10192}]+/u',
				'', $value
			);
		}

		return $value;
	}




	////////////////////////////////////////////////////////////////////////////
	// PHP MAGIC METHOD - WE ARE A READ/DELETE ONLY INSTANCE, THROW EXCEPTION
	////////////////////////////////////////////////////////////////////////////
	public function __set($key, $value) {
		throw new Exception('Cannot set values on class getvar');
	}




	////////////////////////////////////////////////////////////////////////////
	// ARRAYACCESS - WE ARE A READ/DELETE ONLY INSTANCE, THROW EXCEPTION
	////////////////////////////////////////////////////////////////////////////
	public function _offsetSet($key, $value) {
		throw new Exception('Cannot set values on class getvar');
	}




	////////////////////////////////////////////////////////////////////////////
	// PHP MAGIC METHOD - GET THE $_GET/$_POST VALUE FOR THE GIVEN $KEY
	////////////////////////////////////////////////////////////////////////////
	public function __get($key) {
		return $this($key);
	}




	////////////////////////////////////////////////////////////////////////////
	// ARRAYACCESS - GET THE $_GET/$_POST VALUE FOR THE GIVEN $KEY
	////////////////////////////////////////////////////////////////////////////
	public function _offsetGet($key) {
		return $this($key);
	}




	////////////////////////////////////////////////////////////////////////////
	// PHP MAGIC METHOD - CHECK TO SEE IF KEY EXISTS IN $_GET/$_POST
	////////////////////////////////////////////////////////////////////////////
	public function __isset($key) {
		if (!($this->_default & _GETVAR_NOPOST)) {
			if (isset($_POST[$key])) return true;
		}

		if (!($this->_default & _GETVAR_NOGET)) {
			if (isset($_GET[$key])) return true;
		}

		return false;
	}




	////////////////////////////////////////////////////////////////////////////
	// ARRAYACCESS - CHECK TO SEE IF $KEY EXISTS IN $_GET/$_POST
	////////////////////////////////////////////////////////////////////////////
	public function _offsetExists($key) {
		return isset($this->{$key});
	}




	////////////////////////////////////////////////////////////////////////////
	// PHP MAGIC METHOD - REMOVE A $KEY FROM $_GET/$_POST
	////////////////////////////////////////////////////////////////////////////
	public function __unset($key) {
		if (!($this->_default & _GETVAR_NOPOST)) {
			unset($_POST[$key]);
		}

		if (!($this->_default & _GETVAR_NOGET)) {
			unset($_GET[$key]);
		}

		unset($_REQUEST[$key]);
	}




	////////////////////////////////////////////////////////////////////////////
	// ARRAYACCESS - REMOVE A $KEY FROM $_GET/$_POST
	////////////////////////////////////////////////////////////////////////////
	public function _offsetUnset($key) {
		unset($this->{$key});
	}




	////////////////////////////////////////////////////////////////////////////
	// GET THE LOCAL PATH OF THE GETVAR LIBRARY
	////////////////////////////////////////////////////////////////////////////
	public static function dir() {
		return __DIR__;
	}




	////////////////////////////////////////////////////////////////////////////
	// MEMBER VARIABLES
	////////////////////////////////////////////////////////////////////////////
	public			$_default;
	private			$_rawget	= NULL;
	private			$_rawpost	= NULL;
	private			$_rawjson	= NULL;
	private			$_type		= NULL;
	public static	$version	= 'Getvar 2.9.1';

}

# GetVar

## About
GetVar is a library for streamlining access to PHP's $_GET and $_POST super
global variables. This simple class eliminates the need to constantly check
**isset($_GET['param'])** every time you want to read a passed in parameter from
the user's client browser. GetVar also simplifies the transition to and from
both GET and POST HTTP methods by automatically checking if a value was passed
into either $_GET or $_POST without having to worry about it as a developer.


### Getting Started
```php
//First, create an instance of the GetVar object
require_once('getvar/getvar.php.inc');
$get = new getvar;
```

### Usage
Assuming the URL: *http://example.com?x=12.5&y=hello*
```php
//Note in these examples that 'z' was never passed in, and so it returns a
//default value for each function call rather than causing a warning

//Get a simple variable using GetVar's invoke function
$x = $get('x');					//returns '12.5'
$y = $get('y');					//returns 'hello'
$z = $get('z');					//returns ''


//Get a string variable. This forces UTF8 encoding
$x = $get->string('x');			//returns '12.5'
$y = $get->string('y');			//returns 'hello'
$z = $get->string('z');			//returns ''


//Get a string variable. Returns NULL instead of '' for missing items
$x = $get->stringNull('x');		//returns '12.5'
$y = $get->stringNull('y');		//returns 'hello'
$z = $get->stringNull('z');		//returns NULL


//Force the data type to integer (strings become 0 if not recognized)
$x = $get->int('x');			//returns 12 (integer)
$y = $get->int('y');			//returns 0 (integer)
$z = $get->int('z');			//returns 0 (integer)


//Force the data type to float (strings become 0.0 if not recognized)
$x = $get->float('x');			//returns 12.5 (float)
$y = $get->float('y');			//returns 0.0 (float)
$z = $get->float('z');			//returns 0.0 (float)
```


### Automatic Trimming
In our testing and production environments, we've found that in 95-99% of our
usage cases, we want to trim whitespace from value before converting or
returning it. It is extremely common for users to have extra white space on
either end of their user input, especially when copying and pasting information
from other web pages or documents. The default behavior of GetVar is to trim
the string first, but this can be overwritten for those few exceptions, such as
passwords.

Note that we use
[PHP's trim() function](http://php.net/manual/en/function.trim.php)
which will truncate the following characters by default:
* " " (ASCII 32 (0x20)), an ordinary space.
* "\t" (ASCII 9 (0x09)), a tab.
* "\n" (ASCII 10 (0x0A)), a new line (line feed).
* "\r" (ASCII 13 (0x0D)), a carriage return.
* "\0" (ASCII 0 (0x00)), the NUL-byte.
* "\x0B" (ASCII 11 (0x0B)), a vertical tab.

Assuming the URL: *http://example.com?test=%20this%20is%20a%20test%20*
```php
$var = $get('test');					//returns 'this is a test'
$var = $get('test', _GETVAR_NOTRIM);	//returns ' this is a test '
```

Note: This behavior can also be disabled globally when constructing the GetVar
object as follows
```php
$get = new getvar(_GETVAR_NOTRIM);
```


### $_GET vs $_POST
By default GetVar will search $_POST first for a given parameter, then search
$_GET for a given parameter, and then return a default value if the parameter
could not be found. This behavior can be overwritten easily.

```php
//This will scan $_POST, and then $_GET
$name = $get('name');

//This will scan $_POST, but ignore $_GET
$name = $get('name', _GETVAR_NOGET);

//This will scan $_GET, but ignore $_POST
$name = $get('name', _GETVAR_NOPOST);
```

Note: This behavior can also be disabled globally when constructing the GetVar
object as follows
```php
$get = new getvar(_GETVAR_NOGET);
//OR
$get = new getvar(_GETVAR_NOPOST);
```

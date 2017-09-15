
#ifndef ATTR_TYPE_H
#define ATTR_TYPE_H

enum attr_type_t 
{
<?php

error_reporting(E_ALL);

$argc = $_SERVER['argc'];
$argv = $_SERVER['argv'];

$filename = $argv[1];

$xml = simplexml_load_file($filename);
if ($xml === false) {
    exit(1);
}

foreach ($xml as $elem) {

    $attrs = array();

    foreach ($elem->attributes() as $key => $val) {
        $attrs[$key] = $val;
    }

    $id = $attrs['id'];
    $name = $attrs['name'];
    $desc = $attrs['des'];

    $arr = explode('_', $name);
    foreach ($arr as &$a) {
        $a = ucfirst($a); 
    }
    $name = 'k' . implode('', $arr);

    echo "\t/* $desc */\n";
    echo "\t" . $name . ' = ' . $id . ",\n";
}

?>
};

#endif


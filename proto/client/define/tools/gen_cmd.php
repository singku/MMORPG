
#ifndef CLIENT_CMD_H
#define CLIENT_CMD_H

enum cli_cmd_t 
{
<?php

function proc_proto_file($filename)
{
    $file_handle = fopen($filename, "r");
    if (!$file_handle) {
        return;
    }

    while (!feof($file_handle)) {
        $line = fgets($file_handle, 1000000);
        if (preg_match("/^message (cs_(0x[\dA-Fa-f]+)_[^{\s]*)/i", $line, $matches)) {
            //echo $line."\n";
            //var_dump($matches);
            echo "\tcli_cmd_".$matches[1]." = ".hexdec($matches[2]).",\n";
        }
    }
    fclose($file_handle);
}

$argc = $_SERVER['argc'];
$argv = $_SERVER['argv'];

$proto_dir = $argv[1];

$files;
$i = 0;
if (is_dir($proto_dir)) {
    $dir_handle = opendir($proto_dir);
    while (($file = readdir($dir_handle)) !== false) {
        if ($file == "." || $file == ".." || is_dir($file)) {
            continue;
        }
        if (preg_match("/^pb0x.*\.proto$/i", $file)) {
            //echo $file."\n";
            //proc_proto_file($proto_dir."/".$file);
            $files[$i++] = $file;
        }
    }
    sort($files);
    foreach($files as $key=>$file) {
        //echo $file."\n";
        proc_proto_file($proto_dir."/".$file);
    }
}

?>
};

#endif

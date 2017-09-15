
#include "cmd_msg.h"

cmd_msg_t cmd_msg_arr[] = 
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
            //echo "\tcli_cmd_".$matches[1]." = ".hexdec($matches[2]).",\n";

            $in = $matches[1];
            $out = $in;
            $bef = strstr($out, '_');
            $out = "sc".$bef;
            printf("\t{\"onlineproto.%s\", \"onlineproto.%s\", %d},\n", $in, $out, hexdec($matches[2]));
        }
    }
    fclose($file_handle);
    printf("\n");
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
uint32_t cmd_msg_arr_size = sizeof(cmd_msg_arr) / sizeof(cmd_msg_t);

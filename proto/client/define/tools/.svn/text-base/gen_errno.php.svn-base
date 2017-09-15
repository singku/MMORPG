
#ifndef CLIENT_ERRNO_H
#define CLIENT_ERRNO_H

enum cli_errno_t 
{
<?php

$argc = $_SERVER['argc'];
$argv = $_SERVER['argv'];

$filename = $argv[1];

$file_handle = fopen($filename, "r");
if (!$file_handle) {
    return;
}
while (!feof($file_handle)) {
    $line = fgets($file_handle, 10000);
    $line = trim($line);
    $result = preg_split("/[\s]+/", $line);
    //var_dump($result);
    if (count($result) != 3) {
        continue;
    }
    printf("\tcli_err_%s = %d, /* %s */\n", $result[1], $result[0], $result[2]);
}

?>
};

#endif

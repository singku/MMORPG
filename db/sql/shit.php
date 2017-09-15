<?php
    $a = $argv[1];
    $b = $argv[2];
    $c = $a << 32 | $b;
    echo $c."\n";
?>

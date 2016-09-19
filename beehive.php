<?php
$br = (php_sapi_name() == "cli")? "":"<br>";

$arg = [
    'dst_mode' => 0,
    'dst' => 1121,
    'body' => 'this is data',
    'routers_list' => [
        1111, 333
    ]
];

$a = beehive_packet_pack($arg);
var_dump($a);
$ret = beehive_packet_unpack($a);
var_dump($ret);

// if(!extension_loaded('beehive')) {
// 	dl('beehive.' . PHP_SHLIB_SUFFIX);
// }
// $module = 'beehive';
// $functions = get_extension_funcs($module);
// echo "Functions available in the test extension:$br\n";
// foreach($functions as $func) {
//     echo $func."$br\n";
// }
// echo "$br\n";
// $function = 'confirm_' . $module . '_compiled';
// if (extension_loaded($module)) {
// 	$str = $function($module);
// } else {
// 	$str = "Module $module is not compiled into PHP";
// }
// echo "$str\n";




?>

<?php
// doc du lieu tu website gui ve
$tansuat = $_POST["tansuat"];

// ket noi database
include("config.php");

// gui data xuong database
$sql = "update pqp set status='$tansuat' where stt=(SELECT max(stt) FROM pqp) ";
mysqli_query($conn, $sql);

// ngat ket noi voi database
mysqli_close($conn);
?>
const std::string MAIN_HTML = R"(
<!DOCTYPE html>
<html lang='en'>
<head>
    <meta charset='utf-8'>
    <meta http-equiv='X-UA-Compatible' content='IE=edge'>
    <meta name='viewport' content='width=device-width, initial-scale=1'>
    <!-- The above 3 meta tags *must* come first in the head; any other head content must come *after* these tags -->
    <!-- <meta http-equiv='refresh' content='1' /> -->
    <script type='text/javascript' src='https://www.gstatic.com/charts/loader.js'></script>
    <script src='https://ajax.googleapis.com/ajax/libs/jquery/2.1.3/jquery.min.js'></script>
    <!--<link href="https://fonts.googleapis.com/icon?family=Material+Icons" rel="stylesheet">-->
    <link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css'>

    <script type='text/javascript'>
        var Heartbeat = 0;
        var Chart;
        var ChartData;

        // Google Charts
        google.charts.load('current', {'packages':['corechart']});

        $(document).ready(function(){
            $.ajaxSetup({ cache: false }); // or iPhones don't get fresh data
        });


        // Chart options.
        // See https://developers.google.com/chart/interactive/docs/reference#dateformatter
        $$$CHART_OPTIONS

        function TempText(t) {
            if(typeof t === "undefined")
                return "";
            else
                return t;
        }

        function UpdateData() {

            if (Heartbeat) {
                // Update value boxes.
                $.ajax({
                    type: 'GET',
                    url: 'index.php',
                    data: 'status',
                    dataType: 'json',
                    cach: 'no-cache',
                    success: function (jsonObj) {
                        $('#curval0').text( TempText(jsonObj["curval0"]) );
                        $('#curval1').text( TempText(jsonObj["curval1"]) );
                        $('#curval2').text( TempText(jsonObj["curval2"]) );
                        $('#curval3').text( TempText(jsonObj["curval3"]) );
                        $('#curval4').text( TempText(jsonObj["curval4"]) );
                        $('#curslope0').text( TempText(jsonObj["slope0"]) );
                        $('#curslope1').text( TempText(jsonObj["slope1"]) );
                        $("#statustext").html(jsonObj["statustext"]);

                        Heartbeat = !Heartbeat;
                        $('#heartbeat').css({'background-color': 'black'});

                        signStr = " ";
                        if (jsonObj.slope0 > 0)
                           signStr = " +";

                        document.title = jsonObj.curval0 + signStr + jsonObj.slope0;
                        // Note that setTimeout() is used instead of setInterval(). This is because setInterval() was causing
                        //    multiple unsynchronized updates which caused Chrome to crash the page.
                        setTimeout(UpdateData, $$$REFRESH_PERIOD);
                    }
                });
            } else {
                // Get the new chart.
                // Note that date objects are not allowed in JSON.parse but they *are* handled in DataTable().
                // Because of this we cannot combine other data with the chart data.
                $.ajax({
                    type: 'get',
                    url: 'index.php',
                    data: 'chart',
                    dataType: 'json',
                    // If the client table is valid and has changed since the last update..
                    success: function (jsonObj) {
                        ChartData = new google.visualization.DataTable(jsonObj);
                        if ( jQuery.isEmptyObject(Chart) )
                            Chart = new google.visualization.LineChart(document.getElementById('chart'));
                        Chart.draw(ChartData, ChartOptions);
                        ChartData = null;
                        delete ChartData;
//                       delete window.chart;

                        Heartbeat = !Heartbeat;
                        $('#heartbeat').css({'background-color': 'red'});

                        // Note that setTimeout() is used instead of setInterval(). This is because setInterval() was causing
                        //    multiple unsynchronized updates which caused Chrome to crash the page.
                        setTimeout(UpdateData, $$$REFRESH_PERIOD);
                    }
                });
            }
        }
    </script>

    <style>
        #content {
            display:inline-block;
            border: 1px solid black;
            height:200px;
            width:605px;
        }
        #chart {
            width:500px;
            float:left;
            height: 100%;
        }
        #panel {
            height:100%;
            width:100px;
            display:block;
            float:left;
            background:black;
        }
        .row {
            height:40px;
            width:100%;
            margin:0; padding:0;
            float:left;
            text-align: right;
        }
        .valuetext {
            display:inline;
            padding-right:5px;
            color:white;
            font-family:Arial, Helvetica, sans-serif;
            font-weight:bold;
            font-size:30px;
        }
        #heartbeat {
            position:absolute;
            top:1px; left:1px;
            float:left;
            height:10px;
            width:10px;
            background-color: red;
        }
        #configtext {
            font-size:14px;
            font-family:Arial, Helvetica, sans-serif;
            color:black;
        }
        .mybutton {
            border:none;
            height:30px;
            border-radius: 5px;
        }
        #status-block {
            clear:both;
        }
        #status-block2 {
            clear:both;
        }

        /* iPhone 5 Landscape */
        @media only screen
        and (min-device-width: 320px)
        and (max-device-width: 568px)
        and (-webkit-min-device-pixel-ratio: 2)
        and (orientation: landscape) {
            #content {
                height:260px;
            }
            #chart {
                width:465px;
            }
        }

        /* iPhone 5 Portrait */
        @media only screen
        and (min-device-width: 320px)
        and (max-device-width: 320px)
        and (-webkit-min-device-pixel-ratio: 2)
        and (orientation: portrait) {
            #content {
                height:260px;
            }
            #chart {
                width:215px;
            }
        }

    </style>
    <script src="chart_options.js"></script>
</head>

<body style='margin:0;padding:0;' onload='UpdateData()'>
<div id='content'>
    <div id='heartbeat' style=''></div>
    <div id='panel'>
        <div class='row' style='background-color: green;'><span id='curval0' class='valuetext'>$$$CURVAL0</span></div>
        <div class='row' style='background-color: blue;'><span id='curval1' class='valuetext'>$$$CURVAL1</span></div>
        <div class='row' style='background-color: purple;'><span id='curval2' class='valuetext'>$$$CURVAL2</span></div>
        <div class='row' style='background-color: brown;'><span id='curval3' class='valuetext'>$$$CURVAL3</span></div>
        <div class='row' style='background-color: red;'><span id='curval4' class='valuetext'>$$$CURVAL4</span></div>
    </div>
    <div id='chart'>NO VALUES</div>
    <div id='status-block'>
        <div id='statustext'>$$$STATUS_TEXT</div>
        <button type='button' class='btn-md btn-primary mybutton' onclick='window.location.href ="index.php?settingspage";'>SETTINGS</button>
        <button type='button' class='btn-md btn-danger mybutton' style='width:50px' onclick='window.location.href ="index.php?zoomout=1";'>+</button>
        <button type='button' class='btn-md btn-danger mybutton' style='width:50px' onclick='window.location.href ="index.php?zoomin=1";'>-</button>
        <!-- <button type='button' class='btn-xs btn-warning mybutton' onclick='window.location.href ="index.php?testdata=1";'>TEST</button> -->
    </div>
    <div id='status-block2'>
    </div>
</div>
</body>
</html>
)";
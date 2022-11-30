/**
 * Return the chart options.
 */
const std::string CHART_OPTIONS = R"(
      // Chart options.
      // See https://developers.google.com/chart/interactive/docs/reference#dateformatter
      var ChartOptions = {
            chartArea: { left: '5%', top: '5%', bottom:'20%', width: '90%', height: '90%' },
         curveType: 'function',
         legend: { position: 'bottom' },
         hAxis: {
				format: 'h:mm a',
				number: 'none'},
			gridlines: {
				 count: -1
			},
			series:{
				 0:{targetAxisIndex:$$$SENSOR_0_AXIS, color:'green'},
				 1:{targetAxisIndex:$$$SENSOR_1_AXIS, color:'blue'},
				 2:{targetAxisIndex:$$$SENSOR_2_AXIS, color:'purple'},
				 3:{targetAxisIndex:$$$SENSOR_3_AXIS, color:'brown'},
				 4:{targetAxisIndex:$$$SENSOR_4_AXIS, color:'red'}
			},
			vAxes: {
//				viewWindow: {
//					 min: 30,
//					 max: 80
//				}
				0: {minValue: $$$VAXES_0_MIN,maxValue: $$$VAXES_0_MAX},
				1: {minValue: $$$VAXES_1_MIN,maxValue: $$$VAXES_1_MAX}
//                ticks: [0,10,20,30,40,50,60,70,80,90]
			}
		};
	)";

const std::string SETTINGS_HTML = R"(
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
    <link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css">

    <script type='text/javascript'>
        function CheckboxChanged(elem) {
            var e = $(elem);

            var c="0";
            if (e.is(':checked'))
                c = "1";
            var dataStr = e.attr("id") + "=" + c;
//alert(dataStr);

            $.ajax({
                type: 'GET',
                url: 'index.php',
                data: dataStr,
                dataType: 'text',
                cach: 'no-cache',
                success: function (response) {
                }
            });
        }

        function SelectChanged(elem,low,high) {
            var e = $(elem);

            // If elem is within range..
            if (high > low && elem.value >= low && elem.value <= high) {
                var dataStr = e.attr("id") + "=" + elem.value;
//                alert(dataStr);

                $.ajax({
                    type: 'GET',
                    url: 'index.php',
                    data: e.attr("id") + "=" + elem.value,
                    dataType: 'text',
                    cach: 'no-cache',
                    success: function (response) {
                        //                    $('#curval1').text(response);
                    }
                });
            }
        }
        function MenuChanged(elem, valname, channelNum) {
            var e = $(elem);

//                alert(dataStr);

            $.ajax({
                type: 'GET',
                url: 'index.php',
                data: "channel=" + channelNum + "&" + valname + "=" + elem.value,
                dataType: 'text',
                cach: 'no-cache',
                success: function (response) {
                    //                    $('#curval1').text(response);
                }
            });
        }
        // $$$ Need to test for spaces!
        function HostNameChanged(elem) {
            var e = $(elem);

            $.ajax({
                type: 'GET',
                url: 'index.php',
                data: e.attr("id") + "=" + elem.value,
                dataType: 'text',
                cach: 'no-cache',
                success: function (response) {
                }
            });
        }
    </script>

    <style>
        #content {
            display:inline-block;
            height:200px;
            margin-left:5px;
        }
        .section {

        }
        .chan_hdr {
            color:white;
            background-color: black;
            font-weight: bold;
            padding:2px;
            margin-bottom:2px;
        }
        .section1 {
            padding:0 0 3px 0;
            border:1px solid grey;
        }
        .section-label {
            color:white;
            background-color: black;
            font-weight: bold;
            padding:2px;
            margin-bottom:2px;
        }
        #title {
            font-size:20px;
            font-weight:bold;
        }

    </style>
</head>

<body style='margin:0;padding:0;'>
<div id='content'>
    <div id='title'>Temp Logger Settings</div>
    <div class='section'>Enable Logging <input type='checkbox' id='enablelogging' $$$LOGGING_ENABLE onchange='CheckboxChanged(this)'></div>
    <div class='section'>Hostname <input type='text' value='$$$HOSTNAME' id='hostname' size='10' maxlength='19' onchange='HostNameChanged(this)'></input></div><br />
    <div class='section'>Sample Period <select id='sampleperiod' class='Selector' onchange='SelectChanged(this,10,6000)'>$$$SAMPLE_PERIOD_OPTIONS</select></div><br />
    <div class='section'>Chart Samples <input type='text' value='$$$CHARTSAMPLES' id='chartsamples' size='5' onchange='SelectChanged(this,100,2000)'></input></div><br />
    <div class='section'>Slope Time (sec) <input type='text' value='$$$SLOPETIMESEC' id='slopetimesec' size='5' onchange='SelectChanged(this,1,7200)'></input></div><br />
    <div class='section1'>
        <div class='section-label'>Vertical Axis 1</div>
        <div>
            Low <input type='text' id='chartlowtemp1' size='5' value='$$$CHARTLOWTEMP1' onchange='SelectChanged(this,-30,4000)'></input>
            High <input type='text' id='charthightemp1' size='5' value='$$$CHARTHIGHTEMP1' onchange='SelectChanged(this,-30,4000)'></input>
        </div>
    </div><br />
    <div class='section1'>
        <div class='section-label'>Vertical Axis 2</div>
        <div>
            Low <input type='text' id='chartlowtemp2' size='5' value='$$$CHARTLOWTEMP2' onchange='SelectChanged(this,-30,4000)'></input>
            High <input type='text' id='charthightemp2' size='5' value='$$$CHARTHIGHTEMP2' onchange='SelectChanged(this,-30,4000)'></input>
        </div>
    </div><br />
    <div class='section'><div class='chan_hdr'>Channel 1 </div>
        Axis <select id='axis' class='Selector' onchange='MenuChanged(this,"axis",0)'>$$$AXES_OPTIONS_0</select>
    </div><br />
    <div class='section'><div class='chan_hdr'>Channel 2 </div>
        Axis <select id='axis' class='Selector' onchange='MenuChanged(this,"axis",1)'>$$$AXES_OPTIONS_1</select>
    </div><br />
    <div class='section'><div class='chan_hdr'>Channel 3 </div>
        Axis <select id='axis' class='Selector' onchange='MenuChanged(this,"axis",2)'>$$$AXES_OPTIONS_2</select>
    </div><br />
    <div class='section'><div class='chan_hdr'>Channel 4 </div>
        Axis <select id='axis' class='Selector' onchange='MenuChanged(this,"axis",3)'>$$$AXES_OPTIONS_3</select>
    </div><br />
    <div class='section'><div class='chan_hdr'>Channel 5 </div>
        Axis <select id='axis' class='Selector' onchange='MenuChanged(this,"axis",4)'>$$$AXES_OPTIONS_4</select>
    </div><br />
    <button type='button' class='btn btn-primary mybutton' onclick="window.location.href = 'index.php';">SAVE</button>
</div>
</body>
</html>
)";
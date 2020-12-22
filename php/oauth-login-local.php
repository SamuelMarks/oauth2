<?php

// Register an OAuth app with IBM AppID
// Reference: https://developer.okta.com/blog/2018/07/16/oauth-2-command-line
//
// When you register an application with AppID you get a set of details
// {
//     "clientId": "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXX",
//     "tenantId": "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXX",
//     "secret": "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
//     "name": "Sample Node Web Application",
//     "oAuthServerUrl": "https://eu-gb.appid.cloud.ibm.com/oauth/v4/<tenant-id>",
//     "profilesUrl": "https://eu-gb.appid.cloud.ibm.com",
//     "discoveryEndpoint": "https://eu-gb.appid.cloud.ibm.com/oauth/v4/<tenant-id>/.well-known/openid-configuration",
//     "type": "regularwebapp",
//     "scopes": []
// }
//
// The redirect url can be set under Manage Authentication:
//    http://localhost:3000/*
// Example: redirect url is "http://localhost:3000/ibm/cloud/appid/callback"

// Here we make a call to a URL using the CUrl library.
// https://cloud.ibm.com/apidocs/app-id/auth#app-id-access-token
// @param headers   Once authenticated if requires Bearer authentication use this argument to pass to server
// @return json response
function http($url, $params=false, $headers=false) {
    $ch = curl_init($url);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
    if($headers) 
        curl_setopt($ch, CURLOPT_HTTPHEADER, $headers);
    if($params)
        curl_setopt($ch, CURLOPT_POSTFIELDS, http_build_query($params));
    return json_decode(curl_exec($ch));
}


// [INSECURE] These client details should not be kept on a client machine 
// as they are easily compromised.
// How then can we do this?
$client_id = '<complete this>';
$client_secret = '<complete this>';
$metadata = http('https://eu-gb.appid.cloud.ibm.com/oauth/v4/<tenant-id>/.well-known/openid-configuration');

// Set global variables for the small web client we are going to set up.
// This sounds grand but basically its a socket on the port that we will
// listen for a response on.
$ip = '127.0.0.1';
$port = '3000';

$redirect_uri = 'http://localhost:'.$port.'/ibm/cloud/appid/callback';
$socket_str = 'tcp://'.$ip.':'.$port;

// Generate a random state variable which will be created for each run
// Who is this sent between and why?
// Why 5 bytes?
$state = bin2hex(random_bytes(5));

// the http_build_query PHP function generates a URL-encoded query string from an array
// https://cloud.ibm.com/apidocs/app-id/auth#authorization
$authorize_url = $metadata->authorization_endpoint.'?'.http_build_query([
  'response_type' => 'code',
  'client_id' => $client_id,
  'redirect_uri' => $redirect_uri,
  'state' => $state,
  'scope' => 'openid'
]);

// You then need to open a browser window to go to the login screen
// this is where the authoriser takes over the login process.
// Your application needs to wait until we hear back from it.
echo "Open the following URL in a browser to continue\n";
echo $authorize_url."\n";
shell_exec("xdg-open '".$authorize_url."'");

// Start the mini HTTP server and wait for their browser to hit the redirect URL
// Store the query string parameters in a variable
$auth = startHttpServer($socket_str);

// The authorisation provider will send us back our random state
// we check that it is valid otherwise we could be receiving junk from
// an unauthorised server.
if($auth['state'] != $state) {
  echo "Wrong 'state' parameter returned\n";
  exit(2);
}

// we get our authorisation code
$code = $auth['code'];

echo "Received code=$code state=$state\n";

echo "Getting an access token...\n";
// https://cloud.ibm.com/apidocs/app-id/auth#token
$response = http($metadata->token_endpoint, [
  'grant_type' => 'authorization_code',
  'code' => $code,
  'redirect_uri' => $redirect_uri,
  'client_id' => $client_id,
  'client_secret' => $client_secret,
]);

if(!isset($response->access_token)) {
  echo "Error fetching access token\n";
  exit(2);
}

$access_token = $response->access_token;

var_dump($access_token);

// THIS SHOULD BE DONE FROM A KNOWN SERVER.
//      CLIENT SENDS TOKEN
//      WE RETURN TRUE/FALSE
// determine if token is active
// https://cloud.ibm.com/apidocs/app-id/auth#introspect
$response = http("https://eu-gb.appid.cloud.ibm.com/oauth/v4/<tenant-id>/introspect", [
    'token' => $access_token,
    'client_id' => $client_id,
    'client_secret' => $client_secret
]);

var_dump($response);

$response = http("https://eu-gb.appid.cloud.ibm.com/api/v1/attributes", false, 
[
    "Content-type: application/json",
    "Authorization: Bearer ".$access_token
]);

var_dump($response);



echo "Getting the username...\n";
$response = http($metadata->userinfo_endpoint, false,
[
    "Content-type: application/json",
    "Authorization: Bearer ".$access_token
]);

var_dump($response);


function startHttpServer($socketStr) {
  // Adapted from http://cweiske.de/shpub.htm

  $responseOk = "HTTP/1.0 200 OK\r\n"
    . "Content-Type: text/plain\r\n"
    . "\r\n"
    . "Ok. You may close this tab and return to the shell.\r\n";
  $responseErr = "HTTP/1.0 400 Bad Request\r\n"
    . "Content-Type: text/plain\r\n"
    . "\r\n"
    . "Bad Request\r\n";

  ini_set('default_socket_timeout', 60 * 5);

  $server = stream_socket_server($socketStr, $errno, $errstr);

  if(!$server) {
    Log::err('Error starting HTTP server');
    return false;
  }

  do {
    $sock = stream_socket_accept($server);
    if(!$sock) {
      Log::err('Error accepting socket connection');
      exit(1);
    }
    $headers = [];
    $body    = null;
    $content_length = 0;
    //read request headers
    while(false !== ($line = trim(fgets($sock)))) {
      if('' === $line) {
        break;
      }
      $regex = '#^Content-Length:\s*([[:digit:]]+)\s*$#i';
      if(preg_match($regex, $line, $matches)) {
        $content_length = (int)$matches[1];
      }
      $headers[] = $line;
    }
    // read content/body
    if($content_length > 0) {
      $body = fread($sock, $content_length);
    }
    // send response
    list($method, $url, $httpver) = explode(' ', $headers[0]);
    if($method == 'GET') {
      #echo "Redirected to $url\n";
      $parts = parse_url($url);
      #print_r($parts);
      if(isset($parts['path']) && $parts['path'] == '/ibm/cloud/appid/callback'
        && isset($parts['query'])
      ) {
        parse_str($parts['query'], $query);
        if(isset($query['code']) && isset($query['state'])) {
          fwrite($sock, $responseOk);
          fclose($sock);
          return $query;
        }
      }
    }
    fwrite($sock, $responseErr);
    fclose($sock);
  } while (true);
}


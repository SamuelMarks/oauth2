# OAuth2 Client Examples

Why? We want to be able to separate native code on our users devices from code that accesses our database and other resources.  By using OAuth2 we can give our users flexibility in how they authenticate and minimising any sensitive information flowing between the client and server.


## Flow

1. User application requests OAuth2 url from Company server.
2. User application opens browser and lets User sign in.
3. On success, browser will make call to User application web server delivering a code.
4. User application gives the Company server the code and if it is valid the Company server will return a new access token to the User application.
5. User application now uses the access token to access resources including private APIs.


## Services

Using the following IBM services to handle authentication:

* App ID
* IBM Cloud Functions

## Implementing an OAuth2 aware client

1. Create an App ID service.
   1. Recommended reading: https://cloud.ibm.com/docs/appid?topic=appid-adding-overview
2. Ensure the Authentication Settings have http://localhost:3000/* in the "Add web redirect URLs" section.
3. Register your application with the **Applications** section.  It takes a name and a type.  Below are some links for more information:
   1. https://cloud.ibm.com/docs/appid?topic=appid-web-apps
   2. https://cloud.ibm.com/docs/appid?topic=appid-single-page (Deprecated as it used the Implicit Flow which is recognised as not secure)
4. Must be able to send and receive HTTP GET and POST REST API calls.
5. Must be able to start a server to intercept the call back request.
6. Keep track of current access token.

## Testing your private OAuth2 API using curl

**WARNING** Most tutorials have a bug in them for this so watch out!

**WARNING** Do not use ```ibmcloud iam oauth-tokens``` to generate a new oauth token, it will not work.

### Get a bearer token

Get your information from AppID | Applications screen.  If you do not have a application of type regularwebapp then create one.  The application does not actually have to exist, its just a string to associate with the connection details.

Replace \<oAuthServerUrl\>, \<clientId\> and \<secret\> with the relevant details from the connections information.

```
curl -X POST <oAuthServerUrl>/token \
       -H "Authorization: Basic $(echo -n <clientId>:<secret> | base64 --wrap=0)" \
       -H 'Content-Type: application/x-www-form-urlencoded' \
       -d grant_type=client_credentials
       
```

**NOTE**:You need the ```--wrap=0``` otherwise base64 will add in a newline character and you will get the following error:

```
curl: (92) HTTP/2 stream 0 was not closed cleanly: Unknown error code (err 1)
```

### Call your OAuth2 protected API

Once you have this you can then call your private api call, for example:

```
curl --request GET \
  --url https://31f5ff35.eu-gb.apigw.appdomain.cloud/private-authtest/Hello \
  --header 'accept: application/json' \
  --header 'authorization: Bearer REPLACE_BEARER_TOKEN'
```

## Example response variables from the authorisation server

This is intercepted by our local web server.

```
Received 
code=wrbCqScHOMK8VcKcw6nCl8OfwoB2dFVSSSIawoLCuMODw7dIw5rDrcKvwoLDryTDj8KdHE_DsEp2DgbDv05fXcOAw646w4wFOgRuHsKIwoZFwqwKw7R-wpnDsQHCk8K8LcK6worDuMOdXQ4-wqcHwo93UcKXw5vDiWtHwqETFnjDq2wew6gWRiXDqcOmD8KCw7cLLn_DmMK7E8Kaw7zDi8OJOMO-w7UbacKrw6rCpQPDmW4ywo9wZTXCksOCw6k5wrMcKgY_w6FGccOxY2_CvcOtwrPDnUfCqMKZw7zDpkzDrVcFd8KAPGFPC8KVPSJcwpMfw79lwr_CtyUZw4gLw4nDrMK8TMOGZsO0TTzDsMKNw60FwpwKdsOBLEk- 
state=25310e6e8f
```

## Cloud Functions

Before you begin add a new namespace.  **It won't show up until you add at least 1 action in it**, but otherwise you might find the API wrapper around the actions will not show up in the web interface.

Create your packages to collect your api calls in.  In this case we create one for the public actions where the user does not need to be logged in, and one for the private actions where authorisation is required.:

* public
* private

I always suffix my actions with "Action" to distinguish them from the API call, in case they have similar names.

### Public Actions

```
// Name: public/HelloWorld
function main(params) {
	return { message: 'Hello World' };
}
```

Here we are giving the application a public function to get the information it needs to begin the OAuth2 process.  We do this so that this information can change and our application does not need to be updated.

```
// Name: public/GetApplicationEndpoint
// replace <client-id> and <tenant-id> with your specific application values
function main(params) {
	return { clientId: '<client-id>', 
	        endpoint: 'https://eu-gb.appid.cloud.ibm.com/oauth/v4/<tenant-id>', 
	        openid: 'https://eu-gb.appid.cloud.ibm.com/oauth/v4/<tenant-id>/.well-known/openid-configuration' 
	};
}
```

Here is the function that allows our client to access a client token but at no time does the application know our client secret.  It is safe on the server side.

```
// Name: public/GetAccessToken
// Replace <client-secret> with your own application client secret
// Replace <tenant-id> with your application tenant id
async function main(params) {
    params.client_secret='<client-secret>';
    const token_endpoint = 'https://eu-gb.appid.cloud.ibm.com/oauth/v4/<tenant-id>/token';
    const axios = require('axios');
    const response = await axios.post(token_endpoint, params);
    return response.data;   
}
```

Example response:

```
object(stdClass)#3 (5) {
  ["access_token"]=>
  string(1050) "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6ImFwcElkLTMxNDNiOThlLTIwMzUtNGZhYy1iNzY1LTg3OTQwMmExYjU4NS0yMDIwLTEwLTEyVDIwOjU1OjEyLjA0MSIsInZlciI6NH0.eyJpc3MiOiJodHRwczovL2V1LWdiLmFwcGlkLmNsb3VkLmlibS5jb20vb2F1dGgvdjQvMzE0M2I5OGUtMjAzNS00ZmFjLWI3NjUtODc5NDAyYTFiNTg1IiwiZXhwIjoxNjA4NjQ4ODgzLCJhdWQiOlsiMjU1ZDU2OTctOGJkYi00MDQxLTk0ODAtMTU1OGQ0ZTFkZDQ3Il0sInN1YiI6IjE2MmM4MTVkLTFmN2YtNDI0OC1iMTZmLWEwMzQ1Njc0OTkwNyIsImVtYWlsX3ZlcmlmaWVkIjp0cnVlLCJhbXIiOlsiY2xvdWRfZGlyZWN0b3J5Il0sImlhdCI6MTYwODY0NTI4MywidGVuYW50IjoiMzE0M2I5OGUtMjAzNS00ZmFjLWI3NjUtODc5NDAyYTFiNTg1Iiwic2NvcGUiOiJvcGVuaWQgYXBwaWRfZGVmYXVsdCBhcHBpZF9yZWFkdXNlcmF0dHIgYXBwaWRfcmVhZHByb2ZpbGUgYXBwaWRfd3JpdGV1c2VyYXR0ciBhcHBpZF9hdXRoZW50aWNhdGVkIn0.cmBLDM48kTusS-_erq-7kg3K5IKK86ZvM-Lh6UtLvVHPsAgpY1XLPMjOfeestek8QkWEu0Xz3WJCZamsiWkv309cSb5Hw3Syj4HnE9IZI9x7Fp1YxSbi8dNq4V4CCw4uhRexjQQyQXGPvLLxuEYpvF2RmpKp2rHVZDObO00NJdxJARB8iei9E66UjlHOWUBBk3vk2EbVxGHvZGqmC5eGxn1IT_l1rbpGNF0kmmWqlmcHZNmBjUkZ4TZfuNeq5Ww_l648Y8Tkc3UwpNf93APDOAYYWBuUGskWIG76jmnZJCkO90whXyJCzOAbWtRx8A0ISgNb-sY0hqiKEb-_tdqzIw"
  ["expires_in"]=>
  int(3600)
  ["id_token"]=>
  string(1153) "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6ImFwcElkLTMxNDNiOThlLTIwMzUtNGZhYy1iNzY1LTg3OTQwMmExYjU4NS0yMDIwLTEwLTEyVDIwOjU1OjEyLjA0MSIsInZlciI6NH0.eyJpc3MiOiJodHRwczovL2V1LWdiLmFwcGlkLmNsb3VkLmlibS5jb20vb2F1dGgvdjQvMzE0M2I5OGUtMjAzNS00ZmFjLWI3NjUtODc5NDAyYTFiNTg1IiwiYXVkIjpbIjI1NWQ1Njk3LThiZGItNDA0MS05NDgwLTE1NThkNGUxZGQ0NyJdLCJleHAiOjE2MDg2NDg4ODMsInRlbmFudCI6IjMxNDNiOThlLTIwMzUtNGZhYy1iNzY1LTg3OTQwMmExYjU4NSIsImlhdCI6MTYwODY0NTI4MywiZW1haWwiOiJ0ZXJtY2NAZ21haWwuY29tIiwibmFtZSI6IlRlc3QgVGVzdExhc3QiLCJzdWIiOiIxNjJjODE1ZC0xZjdmLTQyNDgtYjE2Zi1hMDM0NTY3NDk5MDciLCJlbWFpbF92ZXJpZmllZCI6dHJ1ZSwiZ2l2ZW5fbmFtZSI6IlRlc3QiLCJmYW1pbHlfbmFtZSI6IlRlc3RMYXN0IiwiaWRlbnRpdGllcyI6W3sicHJvdmlkZXIiOiJjbG91ZF9kaXJlY3RvcnkiLCJpZCI6ImUzMmM2OTAzLWMyMjMtNDFhMi05YWUzLWIyNmY1OGI0NWVhNiJ9XSwiYW1yIjpbImNsb3VkX2RpcmVjdG9yeSJdfQ.X2OS_obhKMF39mQ9m6Ar3mLDfRiKyFrQ1pZLsEjbbPryKZXjDmwDuJw_u2Yypl4p8metZvDsp0_0t6aiVBYkVI5rnstCYyitZkhc-FObBe3U2pNDFUfx4Jn_KazSZLKYwUWaYMyd4QxaWwqxVTgqpuw9OUpK1aIsHDGkyi3xqD4WQD9RpyzDJRNW8RYAOsHDel6EzHowKTj-MmR-Rau8JNAgpTNl6sYU7SNfTAmBbuB5kONdLNHkW9Qj206paEMhRtfi2ZUnGTEFUDwuDLAWwkFKBrjtmFYxJZjF8TbfUhNF4XArPuRN4kQS5vC5jXG52NgP3hXTHN371M68hn2NAQ"
  ["scope"]=>
  string(97) "openid appid_default appid_readuserattr appid_readprofile appid_writeuserattr appid_authenticated"
  ["token_type"]=>
  string(6) "Bearer"
}
```

Here we execute the Introspection endpoint that can validate a token.

```
// Name: public/IntrospectAction
// Replace <client-secret> with your own application client secret
// Replace <tenant-id> with your application tenant id
async function main(params) {
    params.client_secret='<client-secret>';
    const introspect_endpoint = 'https://eu-gb.appid.cloud.ibm.com/oauth/v4/<tenant-id>/introspect';
    const axios = require('axios');
    const response = await axios.post(introspect_endpoint, params);
    return response.data;
}
```

Example response:

```
object(stdClass)#4 (2) {
  ["active"]=>
  bool(true)
  ["scope"]=>
  string(97) "openid appid_default appid_readuserattr appid_readprofile appid_writeuserattr appid_authenticated"
}
```

### Private Actions

```
// Name: private/HelloPrivateWorldAction
function main(params) {
	return { message: 'Hello Private World' };
}
```

## Cloud Functions API

Create two apis: public-api and private-api.

### Public API

| Path | Verb |Package|Action|
|------|------|-------|--------------|
|helloworld | GET | public | HelloWorldAction | 
|GetApplicationEndpoint | GET | public | GetAcplicationEndpoint | 
|GetAccessToken| POST | public | GetAccessTokenAction | 
|IsValidToken | POST | public | IntrospectAction | 

### Private API

Activate **OAuth user authentication** and set App ID Service to your App ID service that you have created.

| Path | Verb |Package|Action|
|------|------|-------|--------------|
|Hello | GET | private | HelloPrivateWorldAction | 






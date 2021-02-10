/**
  *
  * main() will be run when you invoke this action
  *
  * @param Cloud Functions actions accept a single parameter, which must be a JSON object.
  *
  * @return The output of this action, which must be a JSON object.
  *
  */
  
 async function main(params) {
    // user needs to pass the other parameters 
    params.client_secret='N2RhYTRjZTUtYjBmYy00ZTE1LWFiNjEtMjZiYTA4YjMxZTY5';
    const token_endpoint = 'https://eu-gb.appid.cloud.ibm.com/oauth/v4/3143b98e-2035-4fac-b765-879402a1b585/token';
    // const request = require('request');
    // const returnValue = await new Promise(function(resolve, reject) {
    //     request.post(token_endpoint, params, (error, res, body) => {
    //         if ( !error && res.statusCode == 200 ) {
    //             resolve(body);
    //         } else {
    //             reject(error);
    //         }
    //     })
    // });
    // return returnValue;
    const axios = require('axios');
    const response = await axios.post(token_endpoint, params);
    return response.data;
    //return params;
    
}

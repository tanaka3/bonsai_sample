﻿using Microsoft.Bonsai.SimulatorApi.Client;
using Microsoft.Bonsai.SimulatorApi.Models;
using System;
using Newtonsoft.Json;
using System.Net.Http;
using System.Threading;
using System.Text;

namespace CatchAndRun
{
    class Program
    {
        /// <summary>
        /// 
        /// </summary>
        /// <param name="args"></param>
        /// <remarks>
        /// You can run the example by itself, or pass the predict and http://localhost:5000/v1/prediction url 
        /// </remarks>
        static void Main(string[] args)
        {
            if (args.Length == 0)
                TrainAndAssess();
            else
            {
                if (args[0] == "predict")
                    RunPrediction(args[1]);//assumes the second value is the URL
            }
        }
        /// <summary>
        /// Run the Train or Assessment loop
        /// </summary>
        private static void TrainAndAssess()
        {
            int sequenceId = 1;
            String workspaceName = GetWorkspace();
            String accessKey = GetAccessKey();

            BonsaiClientConfig bcConfig = new BonsaiClientConfig(workspaceName, accessKey);

            BonsaiClient client = new BonsaiClient(bcConfig);

            //the cartpole model
            Model model = new Model();

            // object that indicates if we have registered successfully
            object registered = null;
            string sessionId = "";

            while (true)
            {
                // go through the registration process
                if (registered == null)
                {
                    var sessions = client.Session;

                    SimulatorInterface sim_interface = new SimulatorInterface();

                    //シミュレータ名（Bonsaiにはここの名称が表示される）
                    sim_interface.Name = "CatchAndRun";
                    sim_interface.Timeout = 60;
                    sim_interface.Capabilities = null;

                    // minimum required
                    sim_interface.SimulatorContext = bcConfig.SimulatorContext;

                    var registrationResponse = sessions.CreateWithHttpMessagesAsync(workspaceName, sim_interface).Result;

                    if (registrationResponse.Body.GetType() == typeof(SimulatorSessionResponse))
                    {
                        registered = registrationResponse;

                        SimulatorSessionResponse sessionResponse = registrationResponse.Body;

                        // this is required
                        sessionId = sessionResponse.SessionId;
                    }

                    Console.WriteLine(DateTime.Now + " - registered session " + sessionId);

                }
                else // now we are registered
                {
                    Console.WriteLine(DateTime.Now + " - advancing " + sequenceId);

                    // build the SimulatorState object
                    SimulatorState simState = new SimulatorState();
                    simState.SequenceId = sequenceId; // required
                    simState.State = model.State; // required
                    simState.Halted = model.Halted; // required

                    try
                    {
                        // advance only returns an object, so we need to check what type of object
                        var response = client.Session.AdvanceWithHttpMessagesAsync(workspaceName, sessionId, simState).Result;

                        // if we get an error during advance
                        if (response.Body.GetType() == typeof(EventModel))
                        {

                            EventModel eventModel = (EventModel)response.Body;
                            Console.WriteLine(DateTime.Now + " - received event: " + eventModel.Type);
                            sequenceId = eventModel.SequenceId; // get the sequence from the result

                            // now check the type of event and handle accordingly

                            if (eventModel.Type == EventType.EpisodeStart)
                            {

                                Config config = new Config();

                                // use eventModel.EpisodeStart.Config to obtain values
                                dynamic startConfig = eventModel.EpisodeStart.Config;

                                #region 環境値の入力（今回は未使用）
                                if (startConfig.catch_position_x != null)
                                {
                                    config.catch_position_x = startConfig.catch_position_x;
                                }

                                if (startConfig.catch_position_y != null)
                                {
                                    config.catch_position_y = startConfig.catch_position_y;
                                }

                                if (startConfig.run_position_x != null)
                                {
                                    config.run_position_x = startConfig.run_position_x;
                                }

                                if (startConfig.catch_position_y != null)
                                {
                                    config.catch_position_y = startConfig.run_position_y;
                                }
                                #endregion

                                model.Start(config);

                            }
                            else if (eventModel.Type == EventType.EpisodeStep)
                            {
                                Action action = new Action();

                                dynamic stepAction = eventModel.EpisodeStep.Action;

                                //bonsaiから入力された移動先
                                action.catch_position_x = stepAction.catch_position_x;
                                action.catch_position_y = stepAction.catch_position_y;

                                //入力値をもとにシミュレートする
                                model.Step(action);
                            }
                            else if (eventModel.Type == EventType.EpisodeFinish)
                            {
                                Console.WriteLine("Episode Finish");
                            }
                            else if (eventModel.Type == EventType.Idle)
                            {
                                Thread.Sleep(Convert.ToInt32(eventModel.Idle.CallbackTime) * 1000);
                            }
                            else if (eventModel.Type == EventType.Unregister)
                            {
                                try
                                {
                                    client.Session.DeleteWithHttpMessagesAsync(workspaceName, sessionId).Wait();
                                }
                                catch (Exception ex)
                                {
                                    Console.WriteLine("cannot unregister: " + ex.Message);
                                }
                            }
                        }
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine("Error occurred at " + DateTime.UtcNow + ":");
                        Console.WriteLine(ex.ToString());
                        Console.WriteLine("Simulation will now end");
                        Environment.Exit(0);
                    }
                }
            }
        }

        private static void RunPrediction(string predictionurl)
        {
            Model model = new Model();

            HttpClient bc = new HttpClient();

            while (true)
            {
                // create the request object -- using random numbers in the state range
                string reqJson = JsonConvert.SerializeObject(model.State);

                var request = new HttpRequestMessage()
                {
                    Method = HttpMethod.Get,
                    RequestUri = new Uri(predictionurl),
                    Content = new StringContent(reqJson, Encoding.UTF8, "application/json")
                };

                var resp = bc.SendAsync(request).Result;
                resp.EnsureSuccessStatusCode();

                var respJson = resp.Content.ReadAsStringAsync().Result;

                // convert the json string to a dynamic object with Kp and Ki types
                Action resultObj = (Action)JsonConvert.DeserializeObject(respJson);

                //Console.WriteLine($"Kp = {resultObj.}");
                model.Step(resultObj);
            }

        }

        private static string GetWorkspace()
        {

            if (Environment.GetEnvironmentVariable("SIM_WORKSPACE") != null)
            {
                return Environment.GetEnvironmentVariable("SIM_WORKSPACE");
            }

            //fill in your Bonsai workspace ID
            return "<your_bonsai_workspace_id>";
        }

        private static String GetAccessKey()
        {

            if (Environment.GetEnvironmentVariable("SIM_ACCESS_KEY") != null)
            {
                return Environment.GetEnvironmentVariable("SIM_ACCESS_KEY");
            }

            //fill in your Bonsai access key
            return "<your_bonsai_access_key>";
        }
    }
}

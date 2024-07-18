#include "controller/include/onnx_handler.hpp"

OnnxHandler::OnnxHandler(const std::string _path, const int _num_inputs, const int _num_ouputs)
    : path(_path), num_inputs(_num_inputs), num_outputs(_num_ouputs)
{

  // onnx runtime sessions options
  Ort::SessionOptions options;
  OrtCUDAProviderOptions cuda_options;

  cuda_options.device_id = 0;
  cuda_options.gpu_mem_limit = SIZE_MAX;
  cuda_options.arena_extend_strategy = 0;
  cuda_options.do_copy_in_default_stream = 0;
  cuda_options.has_user_compute_stream = 0;
  cuda_options.user_compute_stream = nullptr;
  cuda_options.default_memory_arena_cfg = nullptr;
  options.AppendExecutionProvider_CUDA(cuda_options);

  // create session
  session = Ort::Session{env, path.c_str(), options};

  input_shape = {num_inputs};
  output_shape = {num_outputs};

  input_buffer = std::vector<float>(_num_inputs);
  output_buffer = std::vector<float>(_num_ouputs);
  auto memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);

  input_tensor = Ort::Value::CreateTensor<float>(
      memory_info, input_buffer.data(), input_buffer.size(), input_shape.data(),
      input_shape.size());

  output_tensor = Ort::Value::CreateTensor<float>(
      memory_info, output_buffer.data(), output_buffer.size(), output_shape.data(),
      output_shape.size());

  auto memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
  std::cout << "Memory information : \n"
            << memory_info << std::endl;

  std::cout << "Input and output buffers have been initiliazed!\n"
            << "Address of the controller :\t" << memory_info << std::endl
            << "Input tensor " << input_tensor << "\t tensor size: " << input_buffer.size() << std::endl
            << "Output tensor " << output_tensor << "\t tensor size: " << output_buffer.size() << std::endl;
}

int OnnxHandler::run()
{
  try
  {
    const char *input_names[] = {"observations"};
    const char *output_names[] = {"actions"};

    int num_input_tensors = 1;
    int num_output_tensors = 1;

    auto result = session.Run(opt,
                              input_names, &input_tensor, num_input_tensors,
                              output_names, &output_tensor, num_output_tensors);
    if (result != nullptr)
      throw Ort::Exception(result, "Error running the model");

    return 0;
  }
  catch (const Ort::Exception &e)
  {
    RCLCPP_ERROR(rclcpp::get_logger("OnnxController"), "Error in run: %s", e.what());
    return -1;
  }

  const char *input_names[] = {"observations"};
  const char *output_names[] = {"actions"};

  int num_input_tensors = 1;
  int num_output_tensors = 1;

  session.Run(opt,
              input_names, &input_tensor, num_input_tensors,
              output_names, &output_tensor, num_output_tensors);

  return 0;
}
const std::vector<float> &OnnxHandler::get_input_buffer() const
{
  return input_buffer;
}
const std::vector<float> &OnnxHandler::get_output_buffer() const
{
  return output_buffer;
}
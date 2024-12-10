import subprocess
import threading
import time
import sys

def run_verify(cmd):
    """运行 verify 进程"""
    try:
        process = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return process
    except Exception as e:
        print(f"启动 verify 进程时出错: {e}")
        return None

def run_main(cmd):
    """运行主程序"""
    try:
        process = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return process
    except Exception as e:
        print(f"启动主程序时出错: {e}")
        return None

def monitor_process(process, name):
    """监控进程输出"""
    while True:
        output = process.stdout.readline()
        if output == b'' and process.poll() is not None:
            break
        if output:
            print(f"{name}: {output.strip().decode('utf-8')}")
    
    # 打印错误输出
    _, stderr = process.communicate()
    if stderr:
        print(f"{name} 错误输出: {stderr.decode('utf-8')}")

def main():
    # 构建命令
    verify_cmd = "./verifyClient --verify-only"  # verify 命令
    main_cmd = "./verifyClient"                  # 主程序命令

    # 启动 verify 进程
    verify_process = run_verify(verify_cmd)
    if not verify_process:
        print("启动 verify 进程失败")
        return

    # 启动主程序
    main_process = run_main(main_cmd)
    if not main_process:
        print("启动主程序失败")
        verify_process.terminate()
        return

    # 创建监控线程
    verify_monitor = threading.Thread(target=monitor_process, args=(verify_process, "Verify"))
    main_monitor = threading.Thread(target=monitor_process, args=(main_process, "Main"))

    verify_monitor.start()
    main_monitor.start()

    try:
        # 等待程序执行完成
        verify_monitor.join()
        main_monitor.join()
    except KeyboardInterrupt:
        print("\n收到中断信号，正在终止程序...")
        verify_process.terminate()
        main_process.terminate()
        sys.exit(1)

if __name__ == "__main__":
    main()
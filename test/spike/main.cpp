#include <iostream>
#include "engine.h"

#ifdef USE_EIGEN

void test_cases() {
    std::cout << "[INFO] mode = WITH Eigen." << std::endl;

    // do something here #1:
    std::cout << "\n------ TEST #1 ------" << std::endl;
    {
        Eigen::MatrixXi m_in(3, 2);
        m_in << 1, 2, 
            3, 4, 
            5, 6;
        Eigen::MatrixXd m_out(2, 3);
        array_handle("test", "ops_single", m_out, m_in);

        std::cout << "[C++] Matrix_in = \n" << m_in << std::endl;
        std::cout << "[C++] Matrix_out = \n" << m_out << std::endl;
    }

    // do something here #2:
    std::cout << "\n------ TEST #2 ------" << std::endl;
    {
        Eigen::MatrixXi m_in1(3, 2);
        m_in1 << 1, 2, 
            3, 4, 
            5, 6;
        Eigen::MatrixXi m_in2(2, 3);
        m_in2 << -3, -3, -2, 
            -2, -1, -1;
        Eigen::MatrixXd m_out(2, 3);
        array_handle("test", "ops_dual", m_out, m_in1, m_in2);

        std::cout << "[C++] Matrix_in1 = \n" << m_in1 << std::endl;
        std::cout << "[C++] Matrix_in2 = \n" << m_in2 << std::endl;
        std::cout << "[C++] Matrix_out = \n" << m_out << std::endl;
    }

    // do something here #3:
    std::cout << "\n------ TEST #3 ------" << std::endl;
    {
        int32_t *raw_in = new int32_t[6];
        for(int i = 0; i < 6; ++i) raw_in[i] = i + 1;

        int32_t *raw_out;
        size_t out_rows = 2, out_cols = 3;
        
        array_handle("test", "ops_single", &raw_out, out_rows, out_cols, raw_in, 3, 2);

        std::cout << "[C++] Matrix_in = \n";
        for(int i = 0; i < 3; ++i) {
            for(int j = 0; j < 2; ++j) {
                std::cout << raw_in[i * 2 + j] << " ";
            }
            std::cout << std::endl;
        }

        std::cout << "[C++] Matrix_out = \n";
        for(int i = 0; i < out_rows; ++i) {
            for(int j = 0; j < out_cols; ++j) {
                std::cout << raw_out[i * out_cols + j] << " ";
            }
            std::cout << std::endl;
        }
    }

    // do something here #4:
    std::cout << "\n------ TEST #4 ------" << std::endl;
    {
        int32_t raw_in1[6] = {1, 2, 3, 4, 5, 6};        // shape = (3, 2)
        int32_t raw_in2[6] = {-3, -3, -2, -2, -1, -1};  // shape = (2, 3)

        int32_t *raw_out;
        size_t out_rows = 2, out_cols = 3;

        array_handle("test", "ops_dual", &raw_out, out_rows, out_cols, raw_in1, 3, 2, raw_in2, 2, 3);

        std::cout << "[C++] Matrix_in1 = \n";
        for(int i = 0; i < 3; ++i) {
            for(int j = 0; j < 2; ++j) {
                std::cout << raw_in1[i * 2 + j] << " ";
            }
            std::cout << std::endl;
        }

        std::cout << "[C++] Matrix_in2 = \n";
        for(int i = 0; i < 2; ++i) {
            for(int j = 0; j < 3; ++j) {
                std::cout << raw_in2[i * 3 + j] << " ";
            }
            std::cout << std::endl;
        }

        std::cout << "[C++] Matrix_out = \n";
        for(int i = 0; i < out_rows; ++i) {
            for(int j = 0; j < out_cols; ++j) {
                std::cout << raw_out[i * out_cols + j] << " ";
            }
            std::cout << std::endl;
        }
    }
}

#else

void test_cases() {
    std::cout << "[INFO] mode = WITHOUT Eigen." << std::endl;

    // do something here #1:
    std::cout << "\n------ TEST #1 ------" << std::endl;
    {
        int32_t *raw_in = new int32_t[6];
        for(int i = 0; i < 6; ++i) raw_in[i] = i + 1;

        int32_t *raw_out;
        size_t out_rows = 2, out_cols = 3;
        
        array_handle("test", "ops_single", &raw_out, out_rows, out_cols, raw_in, 3, 2);

        std::cout << "[C++] Matrix_in = \n";
        for(int i = 0; i < 3; ++i) {
            for(int j = 0; j < 2; ++j) {
                std::cout << raw_in[i * 2 + j] << " ";
            }
            std::cout << std::endl;
        }

        std::cout << "[C++] Matrix_out = \n";
        for(int i = 0; i < out_rows; ++i) {
            for(int j = 0; j < out_cols; ++j) {
                std::cout << raw_out[i * out_cols + j] << " ";
            }
            std::cout << std::endl;
        }
    }

    // do something here #2:
    std::cout << "\n------ TEST #2 ------" << std::endl;
    {
        int32_t raw_in1[6] = {1, 2, 3, 4, 5, 6};        // shape = (3, 2)
        int32_t raw_in2[6] = {-3, -3, -2, -2, -1, -1};  // shape = (2, 3)

        int32_t *raw_out;
        size_t out_rows = 2, out_cols = 3;

        array_handle("test", "ops_dual", &raw_out, out_rows, out_cols, raw_in1, 3, 2, raw_in2, 2, 3);

        std::cout << "[C++] Matrix_in1 = \n";
        for(int i = 0; i < 3; ++i) {
            for(int j = 0; j < 2; ++j) {
                std::cout << raw_in1[i * 2 + j] << " ";
            }
            std::cout << std::endl;
        }

        std::cout << "[C++] Matrix_in2 = \n";
        for(int i = 0; i < 2; ++i) {
            for(int j = 0; j < 3; ++j) {
                std::cout << raw_in2[i * 3 + j] << " ";
            }
            std::cout << std::endl;
        }

        std::cout << "[C++] Matrix_out = \n";
        for(int i = 0; i < out_rows; ++i) {
            for(int j = 0; j < out_cols; ++j) {
                std::cout << raw_out[i * out_cols + j] << " ";
            }
            std::cout << std::endl;
        }
    }

    // do something here #3:
    std::cout << "\n------ TEST #3 ------" << std::endl;
    {
        int32_t raw_in1[6] = {1, 2, 3, 4, 5, 6};        // shape = (6, 1)
        int32_t raw_in2[6] = {-3, -3, -2, -2, -1, -1};  // shape = (6, 1)

        int32_t *raw_out;
        size_t out_rows = 1, out_cols = 1;

        array_handle("ideal_numerical", "dotp", &raw_out, out_rows, out_cols, raw_in1, 6, 1, raw_in2, 6, 1);

        std::cout << "[C++] Matrix_in1 = \n";
        for(int i = 0; i < 6; ++i) {
            std::cout << raw_in1[i] << " ";
        }
        std::cout << std::endl;

        std::cout << "[C++] Matrix_in2 = \n";
        for(int i = 0; i < 6; ++i) {
            std::cout << raw_in2[i] << " ";
        }
        std::cout << std::endl;

        std::cout << "[C++] Matrix_out = \n";
        for(int i = 0; i < out_rows; ++i) {
            for(int j = 0; j < out_cols; ++j) {
                std::cout << raw_out[i * out_cols + j] << " ";
            }
            std::cout << std::endl;
        }
    }

    // do something here #4:
    std::cout << "\n------ TEST #4 ------" << std::endl;
    {
        int32_t raw_in1[6] = {1, 2, 3, 4, 5, 6};        // shape = (1, 6)
        int32_t raw_in2[6] = {-3, -3, -2, -2, -1, -1};  // shape = (6, 1)

        int32_t *raw_out;
        size_t out_rows = 1, out_cols = 1;

        array_handle("ideal_numerical", "mvm", &raw_out, out_rows, out_cols, raw_in2, 6, 1, raw_in1, 1, 6);

        std::cout << "[C++] Matrix_in1 = \n";
        for(int i = 0; i < 6; ++i) {
            std::cout << raw_in1[i] << " ";
        }
        std::cout << std::endl;

        std::cout << "[C++] Matrix_in2 = \n";
        for(int i = 0; i < 6; ++i) {
            std::cout << raw_in2[i] << " ";
        }
        std::cout << std::endl;

        std::cout << "[C++] Matrix_out = \n";
        for(int i = 0; i < out_rows; ++i) {
            for(int j = 0; j < out_cols; ++j) {
                std::cout << raw_out[i * out_cols + j] << " ";
            }
            std::cout << std::endl;
        }
    }
}

#endif


int main(int argc, char** argv) {
    // initialize -> place in rocc_t():
    init_python_env();

    test_cases();

    // deinitialize -> place in ~rocc_t():
    deinit_python_env();

    return 0;
}

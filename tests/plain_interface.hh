    /**
     * @brief class Fooable
     */
    class Fooable
    {
    public:
        /// void type
        typedef void void_type;
        using type = int;
        static const int static_value = 1;

        /// Does something.
        int foo() const;
        //! Retrieves something else.
        void set_value(int value);
    };

